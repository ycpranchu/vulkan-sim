#!/bin/bash

# 確保 clang-format 已經安裝
if ! command -v clang-format &> /dev/null
then
    echo "clang-format could not be found. Please install it first."
    exit 1
fi

# 獲取當前目錄
THIS_DIR="$(pwd)"
echo "Formatting files in directory: ${THIS_DIR}"

# 檢查 .clang-format 文件是否存在
if [ ! -f "${THIS_DIR}/.clang-format" ]; then
    echo ".clang-format file not found in the current directory."
    exit 1
else
    echo "Using .clang-format from ${THIS_DIR}/.clang-format"
fi

# 定義需要格式化的目錄
DIRECTORIES=(
    "${THIS_DIR}/libcuda"
    "${THIS_DIR}/src"
    "${THIS_DIR}/src/gpgpu-sim"
    "${THIS_DIR}/src/cuda-sim"
    "${THIS_DIR}/src/gpuwattch"
)

# 格式化指定目錄下的 .h 和 .cc 文件
for DIR in "${DIRECTORIES[@]}"; do
    if [ -d "$DIR" ]; then
        echo "Formatting in directory: $DIR"
        clang-format -i "${DIR}"/*.{h,cc}
    else
        echo "Directory $DIR does not exist, skipping."
    fi
done

echo "Formatting completed."
