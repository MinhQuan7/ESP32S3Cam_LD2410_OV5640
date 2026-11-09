#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Pure upload cooldown / backoff state (host-testable, no Arduino deps).
 *
 * Timing uses unsigned wrap-safe deltas: (uint32_t)(now - t).
 */
typedef struct UploadSchedule {
  bool everOk;
  uint32_t lastOkMs;
  uint32_t lastAttemptMs;
  uint32_t backoffMs;
} UploadSchedule;

static inline void uploadScheduleInit(UploadSchedule *s) {
  if (!s) {
    return;
  }
  s->everOk = false;
  s->lastOkMs = 0;
  s->lastAttemptMs = 0;
  s->backoffMs = 0;
}

/**
 * True when a new upload attempt is allowed by period + backoff.
 * Caller must also check IDLE + WiFi + settle separately.
 */
static inline bool uploadScheduleDue(const UploadSchedule *s,
                                     uint32_t nowMs,
                                     uint32_t periodMs) {
  if (!s) {
    return false;
  }

  if (s->backoffMs > 0) {
    if ((uint32_t)(nowMs - s->lastAttemptMs) < s->backoffMs) {
      return false;
    }
  }

  if (!s->everOk) {
    return true;
  }

  return (uint32_t)(nowMs - s->lastOkMs) >= periodMs;
}

/**
 * Record attempt result. On success: clear backoff, stamp lastOk.
 * On failure: double backoff (minRetry..maxRetry), stamp lastAttempt.
 */
static inline void uploadScheduleOnResult(UploadSchedule *s,
                                          bool ok,
                                          uint32_t nowMs,
                                          uint32_t minRetryMs,
                                          uint32_t maxRetryMs) {
  if (!s) {
    return;
  }

  s->lastAttemptMs = nowMs;

  if (ok) {
    s->everOk = true;
    s->lastOkMs = nowMs;
    s->backoffMs = 0;
    return;
  }

  if (s->backoffMs == 0) {
    s->backoffMs = minRetryMs;
  } else {
    uint64_t next = (uint64_t)s->backoffMs * 2ULL;
    if (next > (uint64_t)maxRetryMs) {
      next = maxRetryMs;
    }
    s->backoffMs = (uint32_t)next;
  }

  if (s->backoffMs < minRetryMs) {
    s->backoffMs = minRetryMs;
  }
  if (s->backoffMs > maxRetryMs) {
    s->backoffMs = maxRetryMs;
  }
}

#ifdef __cplusplus
}
#endif
