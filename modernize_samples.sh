#!/bin/bash

# Azure Sphere Samples Modernization Script
# Updates CMakeLists.txt files from SDK 22.09 to 25.04
# Also updates hardware target from mt3620_rdb to avnet_mt3620_sk

echo "🚀 Azure Sphere Samples Modernization Script"
echo "================================================"

# Find all CMakeLists.txt files in the Samples directory
find /workspaces/azure-sphere-samples/Samples -name "CMakeLists.txt" -type f > /tmp/cmake_files.txt

total_files=$(wc -l < /tmp/cmake_files.txt)
echo "Found $total_files CMakeLists.txt files to update"
echo ""

updated_files=0
skipped_files=0

while read -r file; do
    echo "Processing: $file"
    
    # Check if file contains old configuration
    if grep -q 'TOOLS_REVISION "22.09"' "$file" || grep -q 'TARGET_HARDWARE "mt3620_rdb"' "$file"; then
        # Create backup
        cp "$file" "$file.backup"
        
        # Update TOOLS_REVISION from 22.09 to 25.04
        sed -i 's/TOOLS_REVISION "22.09"/TOOLS_REVISION "25.04"/g' "$file"
        
        # Update TARGET_API_SET from 14 to 17 (common upgrade path)
        sed -i 's/TARGET_API_SET "14"/TARGET_API_SET "17"/g' "$file"
        
        # Update hardware target from mt3620_rdb to avnet_mt3620_sk
        sed -i 's/TARGET_HARDWARE "mt3620_rdb"/TARGET_HARDWARE "avnet_mt3620_sk"/g' "$file"
        
        # Remove deprecated function calls (they show warnings but still work)
        # Note: We'll document these for manual review rather than auto-remove
        
        echo "  ✅ Updated (backup saved as $file.backup)"
        ((updated_files++))
    else
        echo "  ⏭️  Skipped (already modern or no target configurations found)"
        ((skipped_files++))
    fi
    echo ""
done < /tmp/cmake_files.txt

echo "================================================"
echo "📊 Modernization Complete!"
echo "   Updated files: $updated_files"
echo "   Skipped files: $skipped_files"
echo "   Total files:   $total_files"
echo ""
echo "💾 Backups created with .backup extension"
echo "🧪 Ready for build testing!"
