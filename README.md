# Contoh Ekstensi Garurda (GNE)

Contoh ekstensi native untuk [Garurda](https://github.com/lnx645/galang)
memakai **GNE** (Garurda Native Extension): menulis ekstensi dalam C
tanpa menyentuh compiler maupun runtime Garurda.

## Syarat

- `gar` **0.5.0 atau lebih baru**
- gcc (Linux/macOS) atau mingw-w64 (Windows, mis. `pacman -S mingw-w64-x86_64-gcc`)
- header `gne.h` — ada di setiap zip rilis Garurda; `build.sh` juga bisa
  mengunduhnya otomatis

## Daftar contoh

| Folder | Yang dibuktikan |
|---|---|
| `hello/` | fungsi, method objek native, `throw` catchable (code+status), callback ke closure Garurda, state antar panggilan |
| `kv/` | key-value dengan state per-modul (`set_data`) + persistensi berkas — pola yang sama untuk ekstensi Database/Redis |

## Menjalankan

```sh
./build.sh          # Linux/macOS
# atau di Windows: .\build.ps1

cd hello
gar run main.ga
```

`use "nama"` mencari ekstensi pada urutan:
`./gne` → setiap direktori `$GNE_PATH` → `~/.garurda/gne`.

## Referensi

- ABI lengkap: [`gne.h`](https://github.com/lnx645/galang/blob/master/include/gne.h)
- Panduan penulis ekstensi: [`docs/id/gne.md`](https://github.com/lnx645/galang/blob/master/docs/id/gne.md)
  ([English](https://github.com/lnx645/galang/blob/master/docs/en/gne.md))
