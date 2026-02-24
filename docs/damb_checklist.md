# DATA-AMB v1 Implementation Plan (Ambassador)

- Baseline repo commit: `59f3040e16fef6cc1e8e2140690e37aa5b6ed992`
- Goal: Sandbox tilemap rendering with camera pan + zoom + resize-aware visible bounds using `.damb` format with TOC.

## 0) Conventions and Ground Rules

- [ ] All `.damb` integers are little-endian
- [ ] TOC is directly after file header
- [ ] TOC entries must be ordered by file offset and non-overlapping
- [ ] Chunks are 8-byte aligned (pad with 0s)
- [ ] Compression stub:
  - [ ] If compression != 0 → reject file with log
- [ ] Runtime keeps only:
  - [ ] Textures
  - [ ] Atlas rect arrays
  - [ ] Map cell arrays
- [ ] Discard raw chunk buffers immediately
- [ ] Rendering uses `SDL_RenderTexture` per visible tile (no batching)

## 1) Add New Files

### 1.1 `src/damb_format.hxx`

- [x] Fixed-width typedefs: `u8`, `u16`, `u32`, `u64`, `i32`
- [x] MAGIC = `"DATA-AMB"`
- [x] `DAMB_VERSION = 1`
- [x] `DAMB_HEADER_SIZE` constant
- [x] `TOC_ENTRY_SIZE` constant
- [x] 4CC chunk types: `IMAG`, `ATLS`, `MAPL`
- [x] `enum CompressionMethod`
- [x] `enum ImageFormat`
- [x] `enum MapEncoding`
- [x] `struct DambHeader`
- [x] `struct DambTocEntry`
- [x] `struct DambChunkHeader`
- [x] `struct DambChunkImagHeader`
- [x] `struct DambChunkAtlsHeader`
- [x] `struct DambAtlasRecordV1`
- [x] `struct DambChunkMaplHeader`
- [x] `struct DambMapCellV1`
- [x] `constexpr Align8(u64)`
- [x] `static_assert(sizeof(DambTocEntry) == TOC_ENTRY_SIZE)`

### 1.2 `src/damb_runtime.hxx`

- [ ] using `ImageId`, `AtlasId`, `LayerId`
- [ ] `struct RuntimeImage`
- [ ] `struct RuntimeAtlas`
- [ ] `struct MapCell`
- [ ] `struct RuntimeLayer`
- [ ] `struct RuntimeScene`
- [ ] `RuntimeImage* FindImage(...)`
- [ ] `RuntimeAtlas* FindAtlas(...)`
- [ ] `RuntimeAtlas* FindAtlasById16(...)`
- [ ] `RuntimeLayer* FindLayer(...)`

### 1.3 `src/damb_loader.hxx`

- [ ] `bool LoadDambScene(path, renderer, outScene)`

### 1.4 `src/damb_packer_stub.hxx`

- [ ] `bool WriteMinimalSandboxDamb(outPath, pngPath)`

## 2) Config Additions

- [ ] `DEFAULT_DAMB_PATH`
- [ ] `DEFAULT_SANDBOX_PNG`
- [ ] `SANDBOX_MAP_W = 512`
- [ ] `SANDBOX_MAP_H = 512`
- [ ] `TILE_W = 50`
- [ ] `TILE_H = 50`
- [ ] `CAMERA_SPEED`
- [ ] `ZOOM_STEP`
- [ ] `ZOOM_MIN`
- [ ] `ZOOM_MAX`
- [ ] Ensure single source of truth for `TILE_W/H`

## 3) Implement Packer Stub

### 3.1 Helpers

- [ ] `WriteExact()`
- [ ] `WritePOD()`
- [ ] `Tell()`
- [ ] `Seek()`
- [ ] `PadTo8()`
- [ ] `ReadFileBytes()`

### 3.2 `WriteMinimalSandboxDamb()`

- [ ] Open output file
- [ ] Read PNG into memory
- [ ] Write placeholder header
- [ ] Create `vector<DambTocEntry>` size 3
- [ ] Write placeholder TOC
- [ ] Align to 8

### 3.3 Write `IMAG` chunk

- [ ] Record offset
- [ ] Write chunk header
- [ ] Write `IMAG` payload header
- [ ] Write PNG blob
- [ ] `PadTo8`
- [ ] Set TOC size

### 3.4 Write `ATLS` chunk

- [ ] Record offset
- [ ] Write chunk header
- [ ] Write `ATLS` payload header
- [ ] Write 5 atlas records
- [ ] `PadTo8`
- [ ] Set TOC size

### 3.5 Write `MAPL` chunk

- [ ] Record offset
- [ ] Write chunk header
- [ ] Write `MAPL` header (`512x512`)
- [ ] Write `512*512` cells row-major
- [ ] `PadTo8`
- [ ] Set TOC size

### 3.6 Finalize

- [ ] Compute `file_size`
- [ ] Seek back and write TOC entries
- [ ] Seek back and finalize header
- [ ] Close file

## 4) Implement Loader

### 4.1 Helpers

- [ ] `FileSize()`
- [ ] `ReadExact()`
- [ ] `ReadPOD()`
- [ ] `Tell()`
- [ ] `Seek()`

### 4.2 `ValidateHeader()`

- [ ] Magic matches
- [ ] Version matches
- [ ] `toc_offset` correct
- [ ] `toc_count` sane
- [ ] `file_size` matches actual

### 4.3 `ReadToc()`

- [ ] Seek to `toc_offset`
- [ ] Read `toc_count` entries

### 4.4 `ValidateTocOrderedAndInBounds()`

- [ ] Offset aligned
- [ ] Offset + size <= fileSize
- [ ] Monotonic offsets
- [ ] First chunk offset matches expected

### 4.5 `LoadChunksSequential()`

- [ ] Seek to first chunk
- [ ] For each TOC entry:
  - [ ] Read chunk header
  - [ ] Dispatch `IMAG/ATLS/MAPL`
  - [ ] Ensure cursor at `offset+size`

## 5) Chunk Loaders

### 5.1 `IMAG`

- [ ] Read `IMAG` payload header
- [ ] Read PNG bytes
- [ ] `CreateTextureFromPngBytes()`
- [ ] Store `RuntimeImage`
- [ ] Free PNG buffer

### 5.2 PNG → Texture

- [ ] Create `SDL_IOStream` over memory
- [ ] Decode PNG
- [ ] Create `SDL_Texture`
- [ ] Cleanup surface + stream

### 5.3 `ATLS`

- [ ] Read `ATLS` header
- [ ] Read records
- [ ] Build `srcRects` vector
- [ ] Build `flags` vector
- [ ] Append `RuntimeAtlas`

### 5.4 `MAPL`

- [ ] Read `MAPL` header
- [ ] Validate encoding Raw
- [ ] Read `w*h` cells
- [ ] Append `RuntimeLayer`

### 5.5 `ValidateMapCells()`

- [ ] atlas exists
- [ ] `asset_index < atlas.srcRects.size()`

## 6) Integrate Into Ambassador

- [ ] Add `RuntimeScene m_scene`
- [ ] `bool m_sceneLoaded`
- [ ] `int m_activeLayerIndex`
- [ ] `float m_cam_x, m_cam_y`
- [ ] `float m_zoom`
- [ ] `float m_zoom_min/max`
- [ ] `int m_visible_min_x, max_x`
- [ ] `int m_visible_min_y, max_y`
- [ ] `loadSandboxDamb()`
- [ ] `onWindowResized()`
- [ ] `onMouseWheel()`
- [ ] `setZoom()`
- [ ] `recomputeVisibleBounds()`
- [ ] `clampCameraToMapWithSlack()`

## 7) Event Wiring

- [ ] Handle `SDL_EVENT_WINDOW_RESIZED`
- [ ] Handle `SDL_EVENT_MOUSE_WHEEL`
- [ ] Optional: WASD camera

## 8) Update Loop

- [ ] Camera movement from input
- [ ] `clampCameraToMapWithSlack()`
- [ ] `recomputeVisibleBounds()`

## 9) Rendering

- [ ] `renderScene()`
- [ ] `renderLayer()`
- [ ] `worldToScreenRect()`
- [ ] Loop visible tiles only
- [ ] `SDL_RenderTexture` per tile

## 10) Visible Bounds Math

- [ ] `worldViewW = logicalW / zoom`
- [ ] `worldViewH = logicalH / zoom`
- [ ] `left = cam_x`
- [ ] `right = cam_x + worldViewW`
- [ ] `minX = floor(left / TILE_W)`
- [ ] `maxX = ceil(right / TILE_W)`
- [ ] Expand by 1 tile margin
- [ ] Clamp to map bounds

## 11) Camera Clamping with Slack

- [ ] `slackPixels` defined
- [ ] `worldW = map.w * TILE_W`
- [ ] `maxCamX = worldW - worldViewW`
- [ ] clamp `cam_x` to `[-slack, maxCamX + slack]`

## 12) Zoom

- [ ] `onMouseWheel()`
- [ ] `setZoom()`
- [ ] clamp zoom to `[ZOOM_MIN, ZOOM_MAX]`
- [ ] recompute bounds

## 13) Smoke Tests

- [ ] `.damb` builds
- [ ] Header validates
- [ ] TOC validates
- [ ] Texture created
- [ ] Atlas rects correct
- [ ] Layer cells correct
- [ ] Camera pans
- [ ] Zoom works
- [ ] Resize updates bounds