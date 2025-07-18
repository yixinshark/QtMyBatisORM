#!/bin/bash
# 修复所有缺少MOC包含的测试文件

files_with_qobject=(
    "tests/test_logger.cpp"
    "tests/test_jsonconfigparser.cpp" 
    "tests/test_sessionfactory.cpp"
    "tests/test_dynamicsqlprocessor.cpp"
    "tests/test_multi_database_integration.cpp"
    "tests/test_cachemanager.cpp"
)

for file in "${files_with_qobject[@]}"; do
    if [ -f "$file" ]; then
        filename=$(basename "$file" .cpp)
        if ! grep -q "#include \"$filename\.moc\"" "$file"; then
            echo "修复 $file"
            # 检查是否已有QTEST_MAIN，如果没有则添加
            if ! grep -q "QTEST_MAIN" "$file"; then
                class_name=$(echo $filename | sed "s/test_/Test/" | sed "s/_\(.\)/\U\1/g")
                echo "" >> "$file"
                echo "QTEST_MAIN($class_name)" >> "$file"
            fi
            echo "#include \"$filename.moc\"" >> "$file"
            echo "已修复 $file"
        fi
    fi
done

