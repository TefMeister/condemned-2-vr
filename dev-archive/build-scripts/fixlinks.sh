#!/usr/bin/env bash
# Git on Windows without symlink support checks out mode-120000 entries as small
# TEXT files containing the link target. Replace each with a real copy.
SDK="E:/condemned-2-vr/src/rexglue-sdk"
cd "$SDK" || exit 1

fixed=0; failed=0

fix_repo() {
  local repo="$1"
  ( cd "$repo" || return
    git ls-files -s 2>/dev/null | awk '$1=="120000"{ $1="";$2="";$3="";sub(/^[ \t]+/,""); print }' | while IFS= read -r rel; do
      [ -z "$rel" ] && continue
      [ -f "$rel" ] || continue
      # a real symlink would not be a tiny regular file; guard on size
      sz=$(stat -c %s "$rel" 2>/dev/null || echo 9999)
      [ "$sz" -gt 500 ] && continue
      target=$(tr -d '\r\n' < "$rel")
      case "$target" in ''|*$'\n'*) continue;; esac
      dir=$(dirname "$rel")
      src="$dir/$target"
      if [ -f "$src" ]; then
        cp -f "$src" "$rel" && echo "FIXED  $repo/$rel  <- $target"
      elif [ -d "$src" ]; then
        rm -f "$rel" && cp -rf "$src" "$rel" && echo "FIXEDD $repo/$rel  <- $target"
      else
        echo "MISS   $repo/$rel  -> $target (target not found)"
      fi
    done
  )
}

echo "### scanning main repo and all submodules for symlink-as-text files"
fix_repo "."
git submodule foreach --recursive --quiet 'echo "$displaypath"' 2>/dev/null | while IFS= read -r sm; do
  [ -n "$sm" ] && fix_repo "$sm"
done

echo "### done"
