#!/usr/bin/env bash

set -ex

cd "$(dirname $0)/.."

if [ -z "$HMCL_LAUNCHER_VERSION" ]; then
  echo "HMCL_LAUNCHER_VERSION unset"
  exit 1
fi

ROOT_DIR="$PWD"
PUBLISH_DIR="$ROOT_DIR/publish"
OUTPUT_DIR="$PUBLISH_DIR/libs"
NAME_BASE="HMCLauncher-$HMCL_LAUNCHER_VERSION"

mkdir -p "$OUTPUT_DIR"

RELEASES_DOWNLOAD="https://github.com/HMCL-dev/HMCLauncher/releases/download"
EXE_FILE="$OUTPUT_DIR/$NAME_BASE.exe"

if [[ ! -f "$EXE_FILE" ]]; then
    wget -O "$EXE_FILE" "$RELEASES_DOWNLOAD/$HMCL_LAUNCHER_VERSION/HMCLauncher.exe"
fi

if [[ ! -f "$EXE_FILE.sha256" ]]; then 
    wget -O "$EXE_FILE.sha256" "$RELEASES_DOWNLOAD/$HMCL_LAUNCHER_VERSION/HMCLauncher.exe.sha256"
fi

echo "$(cat "$EXE_FILE.sha256") $EXE_FILE" | sha256sum --check --status

ASSETS_DIR="$OUTPUT_DIR/assets"
rm -rf "$ASSETS_DIR"
mkdir -p "$ASSETS_DIR"
cp "$EXE_FILE" "$ASSETS_DIR/HMCLauncher.exe"
jar -cvf "$OUTPUT_DIR/$NAME_BASE.jar" -C "$OUTPUT_DIR" assets

cat "$PUBLISH_DIR/pom.xml" | sed -e "s/HMCL_LAUNCHER_VERSION/$HMCL_LAUNCHER_VERSION/" > "$OUTPUT_DIR/$NAME_BASE.pom"
cp -f "$PUBLISH_DIR/empty.jar" "$OUTPUT_DIR/$NAME_BASE-javadoc.jar"
cp -f "$PUBLISH_DIR/empty.jar" "$OUTPUT_DIR/$NAME_BASE-sources.jar"

allSuffixes=('.pom' '.jar' '-javadoc.jar' '-sources.jar')

for suffix in ${allSuffixes[@]}; do
   gpg --yes -ab "$OUTPUT_DIR/$NAME_BASE$suffix"
done

cd "$OUTPUT_DIR"
jar -cvf "$NAME_BASE-bundle.jar" \
    "$NAME_BASE.pom" "$NAME_BASE.pom.asc" \
    "$NAME_BASE.jar" "$NAME_BASE.jar.asc" \
    "$NAME_BASE-javadoc.jar" "$NAME_BASE-javadoc.jar.asc" \
    "$NAME_BASE-sources.jar" "$NAME_BASE-sources.jar.asc"
