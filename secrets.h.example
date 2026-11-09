#pragma once

// Copy this file to secrets.h and fill in real values.
// secrets.h is gitignored — never commit credentials.

#ifndef WIFI_SSID
#define WIFI_SSID "YOUR_WIFI_SSID"
#endif

#ifndef WIFI_PASS
#define WIFI_PASS "YOUR_WIFI_PASSWORD"
#endif

// Hostname ONLY — no "https://", no trailing slash.
// Wrong: "https://xxxx.supabase.co"  → http.begin will fail
// Right: "xxxx.supabase.co"
#ifndef SUPABASE_HOST
#define SUPABASE_HOST "YOUR_PROJECT_REF.supabase.co"
#endif

// Use the anon / publishable key (NOT service_role).
// Legacy JWT starts with "eyJ..."; new keys start with "sb_publishable_...".
#ifndef SUPABASE_KEY
#define SUPABASE_KEY "YOUR_SUPABASE_ANON_OR_PUBLISHABLE_KEY"
#endif

#ifndef SUPABASE_BUCKET
#define SUPABASE_BUCKET "camera-snapshots"
#endif

// Must match the first folder segment in the Supabase Storage RLS policy.
#ifndef DEVICE_ID
#define DEVICE_ID "cam-01"
#endif
