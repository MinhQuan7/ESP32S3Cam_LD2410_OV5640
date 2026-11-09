-- Supabase Storage setup for ESP32-S3 camera snapshots.
-- Run in Supabase Dashboard → SQL Editor.
-- Replace 'cam-01' with your DEVICE_ID if different.

insert into storage.buckets (id, name, public, file_size_limit, allowed_mime_types)
values (
  'camera-snapshots',
  'camera-snapshots',
  false,
  204800,
  '{image/jpeg}'
)
on conflict (id) do nothing;

-- Insert-only for anon role, restricted to path prefix DEVICE_ID/
drop policy if exists "device_insert_snapshots" on storage.objects;

create policy "device_insert_snapshots"
on storage.objects
for insert
to anon
with check (
  bucket_id = 'camera-snapshots'
  and (storage.foldername(name))[1] = 'cam-01'
);

-- No SELECT/UPDATE/DELETE policies for anon — bucket stays private.
-- Dashboard / authenticated app users need separate policies if they must read objects.
