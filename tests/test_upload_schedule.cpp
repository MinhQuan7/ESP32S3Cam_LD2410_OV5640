#include <cassert>
#include <cstdio>
#include <cstdint>

#include "../upload_schedule.h"

static int g_failures = 0;

#define CHECK(cond, msg)                                                       \
  do {                                                                         \
    if (!(cond)) {                                                             \
      std::printf("FAIL: %s\n", msg);                                          \
      ++g_failures;                                                            \
    } else {                                                                   \
      std::printf("PASS: %s\n", msg);                                          \
    }                                                                          \
  } while (0)

static void test_first_attempt_due_immediately() {
  UploadSchedule s;
  uploadScheduleInit(&s);
  CHECK(uploadScheduleDue(&s, 0, 300000), "first attempt due at t=0");
  CHECK(uploadScheduleDue(&s, 1000, 300000), "first attempt due at t=1000");
}

static void test_period_after_success() {
  UploadSchedule s;
  uploadScheduleInit(&s);
  uploadScheduleOnResult(&s, true, 10000, 60000, 300000);
  CHECK(s.everOk, "everOk set after success");
  CHECK(s.backoffMs == 0, "backoff cleared after success");
  CHECK(!uploadScheduleDue(&s, 10000 + 299999, 300000),
        "not due 1ms before period");
  CHECK(uploadScheduleDue(&s, 10000 + 300000, 300000),
        "due exactly at period");
  CHECK(uploadScheduleDue(&s, 10000 + 300001, 300000),
        "due after period");
}

static void test_backoff_doubling() {
  UploadSchedule s;
  uploadScheduleInit(&s);

  uploadScheduleOnResult(&s, false, 1000, 60000, 300000);
  CHECK(s.backoffMs == 60000, "first fail backoff=60s");
  CHECK(!uploadScheduleDue(&s, 1000 + 59999, 300000),
        "not due during first backoff");
  CHECK(uploadScheduleDue(&s, 1000 + 60000, 300000),
        "due after first backoff");

  uploadScheduleOnResult(&s, false, 1000 + 60000, 60000, 300000);
  CHECK(s.backoffMs == 120000, "second fail backoff=120s");

  uploadScheduleOnResult(&s, false, 1000 + 60000 + 120000, 60000, 300000);
  CHECK(s.backoffMs == 240000, "third fail backoff=240s");

  uploadScheduleOnResult(&s, false, 1000 + 60000 + 120000 + 240000, 60000,
                         300000);
  CHECK(s.backoffMs == 300000, "fourth fail backoff capped at 300s");

  uploadScheduleOnResult(&s, false, 1000 + 60000 + 120000 + 240000 + 300000,
                         60000, 300000);
  CHECK(s.backoffMs == 300000, "fifth fail stays at cap");
}

static void test_success_resets_backoff() {
  UploadSchedule s;
  uploadScheduleInit(&s);
  uploadScheduleOnResult(&s, false, 1000, 60000, 300000);
  uploadScheduleOnResult(&s, false, 61000, 60000, 300000);
  CHECK(s.backoffMs == 120000, "precondition backoff=120s");

  uploadScheduleOnResult(&s, true, 200000, 60000, 300000);
  CHECK(s.backoffMs == 0, "success clears backoff");
  CHECK(s.everOk, "everOk after recovery success");
  CHECK(!uploadScheduleDue(&s, 200000 + 299999, 300000),
        "period enforced after recovery");
  CHECK(uploadScheduleDue(&s, 200000 + 300000, 300000),
        "due after period post-recovery");
}

static void test_millis_wraparound() {
  UploadSchedule s;
  uploadScheduleInit(&s);

  const uint32_t nearWrap = 0xFFFFFFF0u;
  uploadScheduleOnResult(&s, true, nearWrap, 60000, 300000);

  // 16 ms after wrap: (0x10 - 0xFFFFFFF0) = 32 decimal — still << period
  CHECK(!uploadScheduleDue(&s, 0x10u, 300000),
        "not due shortly after millis wrap");

  // Exactly period after nearWrap: nearWrap + 300000 wraps
  const uint32_t dueAt = nearWrap + 300000u;
  CHECK(uploadScheduleDue(&s, dueAt, 300000),
        "due at period across wrap");

  // Failure near wrap, backoff must use wrap-safe delta
  UploadSchedule s2;
  uploadScheduleInit(&s2);
  uploadScheduleOnResult(&s2, false, nearWrap, 60000, 300000);
  CHECK(!uploadScheduleDue(&s2, nearWrap + 59999u, 300000),
        "backoff holds across wrap");
  CHECK(uploadScheduleDue(&s2, nearWrap + 60000u, 300000),
        "due after backoff across wrap");
}

int main() {
  test_first_attempt_due_immediately();
  test_period_after_success();
  test_backoff_doubling();
  test_success_resets_backoff();
  test_millis_wraparound();

  if (g_failures != 0) {
    std::printf("\n%d FAILURE(S)\n", g_failures);
    return 1;
  }
  std::printf("\nALL PASS\n");
  return 0;
}
