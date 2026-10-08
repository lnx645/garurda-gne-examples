#!/bin/sh
# Bangun semua contoh ekstensi GNE untuk Linux/macOS.
# Hasil: <contoh>/gne/<nama>.so — langsung bisa `gar run main.ga`.
set -e
cd "$(dirname "$0")"

# Cari header gne.h: env eksplisit → sistem → salinan lokal → unduh.
if [ -n "$GNE_HEADER" ]; then
	INC="$GNE_HEADER"
elif [ -f /usr/local/include/gne.h ]; then
	INC=/usr/local/include
elif [ -f "$HOME/garurda/include/gne.h" ]; then
	INC="$HOME/garurda/include"
else
	mkdir -p .cache
	echo "gne.h tidak ditemukan — mengunduh dari GitHub..."
	curl -fsSL https://raw.githubusercontent.com/lnx645/galang/master/include/gne.h \
		-o .cache/gne.h
	INC=.cache
fi

if [ "$(uname -s)" = "Darwin" ]; then
	FLAGS="-dynamiclib"
else
	FLAGS="-shared -fPIC"
fi

for ex in hello kv; do
	mkdir -p "$ex/gne"
	gcc $FLAGS -Wall -Wextra -I "$INC" -o "$ex/gne/$ex.so" "$ex/$ex.c"
	echo "OK: $ex/gne/$ex.so"
done

echo "Selesai. Coba: cd hello && gar run main.ga"
