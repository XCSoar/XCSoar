#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
# Script to check if all widgets and forms use the same screensize function

set -e

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$PROJECT_ROOT"

echo "Checking screen size function usage in Widgets and Forms..."
echo "=========================================================="
echo ""

# Patterns to search for
PATTERNS=(
    "SystemWindowSize"
    "GetScreenSize"
    "GetClientRect"
    "GetParentClientRect"
    "GetSize\\(\\)"
    "display\\.GetSize"
    "main_window\\.Get"
    "GetWindow\\(\\)\\.Get"
)

# Directories to check
DIRS=(
    "src/Widget"
    "src/Form"
    "src/Dialogs"
)

# Results
declare -A PATTERN_COUNTS
declare -A FILE_PATTERNS

# Initialize counters
for pattern in "${PATTERNS[@]}"; do
    PATTERN_COUNTS["$pattern"]=0
done

echo "Searching for screen size access patterns..."
echo ""

# Search in each directory
for dir in "${DIRS[@]}"; do
    if [[ ! -d "$dir" ]]; then
        echo "Warning: Directory $dir not found, skipping..."
        continue
    fi
    
    echo "Checking $dir:"
    
    for pattern in "${PATTERNS[@]}"; do
        # Use grep to find matches
        matches=$(grep -r -E "$pattern" "$dir" --include="*.cpp" --include="*.hpp" 2>/dev/null | wc -l || echo "0")
        
        if [[ "$matches" -gt 0 ]]; then
            PATTERN_COUNTS["$pattern"]=$((PATTERN_COUNTS["$pattern"] + matches))
            echo "  $pattern: $matches matches"
            
            # Store which files use this pattern
            files=$(grep -r -l -E "$pattern" "$dir" --include="*.cpp" --include="*.hpp" 2>/dev/null || true)
            while IFS= read -r file; do
                if [[ -n "$file" ]]; then
                    FILE_PATTERNS["$file"]="${FILE_PATTERNS["$file"]} $pattern"
                fi
            done <<< "$files"
        fi
    done
    echo ""
done

echo "=========================================================="
echo "Summary of screen size access patterns:"
echo "=========================================================="
echo ""

total_files=0
for pattern in "${PATTERNS[@]}"; do
    count=${PATTERN_COUNTS["$pattern"]}
    if [[ $count -gt 0 ]]; then
        echo "$pattern: $count occurrences"
    fi
done

echo ""
echo "=========================================================="
echo "Files using multiple patterns (potential inconsistencies):"
echo "=========================================================="
echo ""

found_multiple=false
for file in "${!FILE_PATTERNS[@]}"; do
    patterns="${FILE_PATTERNS[$file]}"
    pattern_count=$(echo "$patterns" | wc -w)
    if [[ $pattern_count -gt 1 ]]; then
        found_multiple=true
        echo "$file:"
        echo "  Uses: $patterns"
        echo ""
    fi
done

if [[ "$found_multiple" == false ]]; then
    echo "No files found using multiple patterns."
fi

echo ""
echo "=========================================================="
echo "Detailed analysis by pattern:"
echo "=========================================================="
echo ""

# Check for SystemWindowSize usage (should be minimal)
echo "1. SystemWindowSize() usage:"
syswin_files=$(grep -r -l "SystemWindowSize" "${DIRS[@]}" --include="*.cpp" --include="*.hpp" 2>/dev/null || true)
if [[ -n "$syswin_files" ]]; then
    echo "   WARNING: Found direct SystemWindowSize() usage in:"
    echo "$syswin_files" | while IFS= read -r file; do
        echo "     - $file"
    done
else
    echo "   ✓ No direct SystemWindowSize() usage found"
fi
echo ""

# Check for GetClientRect usage (most common, should be consistent)
echo "2. GetClientRect() usage:"
getclient_files=$(grep -r -l "GetClientRect" "${DIRS[@]}" --include="*.cpp" --include="*.hpp" 2>/dev/null | wc -l || echo "0")
echo "   Found in $getclient_files files"
echo ""

# Check for GetParentClientRect usage
echo "3. GetParentClientRect() usage:"
getparent_files=$(grep -r -l "GetParentClientRect" "${DIRS[@]}" --include="*.cpp" --include="*.hpp" 2>/dev/null | wc -l || echo "0")
echo "   Found in $getparent_files files"
if [[ $getparent_files -gt 0 ]]; then
    echo "   Files using GetParentClientRect():"
    grep -r -l "GetParentClientRect" "${DIRS[@]}" --include="*.cpp" --include="*.hpp" 2>/dev/null | while IFS= read -r file; do
        echo "     - $file"
    done
fi
echo ""

# Check for main_window.Get usage
echo "4. main_window.Get*() usage:"
mainwin_files=$(grep -r -l "main_window\.Get" "${DIRS[@]}" --include="*.cpp" --include="*.hpp" 2>/dev/null | wc -l || echo "0")
echo "   Found in $mainwin_files files"
if [[ $mainwin_files -gt 0 ]]; then
    echo "   Files using main_window.Get*():"
    grep -r -l "main_window\.Get" "${DIRS[@]}" --include="*.cpp" --include="*.hpp" 2>/dev/null | while IFS= read -r file; do
        echo "     - $file"
    done
fi
echo ""

# Check for GetWindow().Get usage (widgets)
echo "5. GetWindow().Get*() usage (widgets):"
getwin_files=$(grep -r -l "GetWindow\(\)\.Get" "${DIRS[@]}" --include="*.cpp" --include="*.hpp" 2>/dev/null | wc -l || echo "0")
echo "   Found in $getwin_files files"
if [[ $getwin_files -gt 0 ]]; then
    echo "   Files using GetWindow().Get*():"
    grep -r -l "GetWindow\(\)\.Get" "${DIRS[@]}" --include="*.cpp" --include="*.hpp" 2>/dev/null | while IFS= read -r file; do
        echo "     - $file"
    done
fi
echo ""

# Check for display.GetSize usage
echo "6. display.GetSize() usage:"
display_files=$(grep -r -l "display\.GetSize" "${DIRS[@]}" --include="*.cpp" --include="*.hpp" 2>/dev/null | wc -l || echo "0")
echo "   Found in $display_files files"
if [[ $display_files -gt 0 ]]; then
    echo "   WARNING: Found direct display.GetSize() usage in:"
    grep -r -l "display\.GetSize" "${DIRS[@]}" --include="*.cpp" --include="*.hpp" 2>/dev/null | while IFS= read -r file; do
        echo "     - $file"
    done
fi
echo ""

echo "=========================================================="
echo "Recommendations:"
echo "=========================================================="
echo ""
echo "Standard pattern should be:"
echo "  - Widgets: GetWindow().GetClientRect() or GetClientRect()"
echo "  - Forms/Dialogs: GetClientRect() or GetParentClientRect()"
echo "  - Main window access: main_window.GetClientRect()"
echo ""
echo "Avoid:"
echo "  - Direct SystemWindowSize() calls (use window methods instead)"
echo "  - Direct display.GetSize() calls (use window methods instead)"
echo ""



