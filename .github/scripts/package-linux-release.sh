#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 3 ]]; then
  echo "Usage: $0 BUILD_DIRECTORY OUTPUT_DIRECTORY VERSION" >&2
  exit 1
fi

build_dir="$1"
output_dir="$2"
version="$3"
# Resolve source files from this script's location without consulting Git. The
# Actions container may run with a different owner from the checkout directory.
repo_root="$(realpath -- "$(dirname -- "${BASH_SOURCE[0]}")/../..")"

if [[ ! "$version" =~ ^[A-Za-z0-9._-]+$ ]]; then
  echo "VERSION must contain only letters, numbers, dots, underscores, or hyphens." >&2
  exit 1
fi

test -x "$build_dir/GoldenEye"
test -f "$build_dir/librexruntime.so"
test -f "$build_dir/librexgpu-xenos.so"

mkdir -p "$output_dir"
staging="$(mktemp -d "$output_dir/.package.XXXXXX")"
trap 'rm -rf -- "$staging"' EXIT
package="$staging/GoldenEye-linux-amd64"
mkdir -p "$package"

# Explicitly select distributable files; never copy game inputs, generated
# sources, local settings, saves, or logs from the build tree.
cp -L "$build_dir/GoldenEye" "$build_dir/librexruntime.so" \
  "$build_dir/librexgpu-xenos.so" "$package/"
if [[ -f "$build_dir/libTracyClient.so" ]]; then
  cp -L "$build_dir/libTracyClient.so" "$package/"
fi
cp "$repo_root/README.md" "$repo_root/LICENSE" "$package/"

# The SDK and executable need a newer libstdc++ than some SteamOS versions
# provide. Bundle the build toolchain's runtimes while using the host's glibc.
compiler="${CXX:-c++}"
for library in libstdc++.so.6 libgcc_s.so.1; do
  runtime_path="$("$compiler" -print-file-name="$library")"
  test -f "$runtime_path"
  cp -L "$runtime_path" "$package/$library"
done

# Catch missing shared-library dependencies before publishing the archive.
for binary in "$package/GoldenEye" "$package/"*.so*; do
  dependencies="$(LD_LIBRARY_PATH="$package" ldd "$binary")"
  if [[ "$dependencies" == *"not found"* ]]; then
    echo "$dependencies" >&2
    exit 1
  fi
done

archive_name="GoldenEye-linux-amd64-${version}.tar.gz"
tar -czf "$output_dir/$archive_name" -C "$staging" GoldenEye-linux-amd64
(
  cd -- "$output_dir"
  sha256sum "$archive_name" > "$archive_name.sha256"
)
echo "Packaged $output_dir/$archive_name"
