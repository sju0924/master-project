#!/bin/bash

# 인자 확인
if [ -z "$1" ]; then
	    echo "Usage: $0 <TEST_CASE_DIR>"
	        exit 1
fi

# TEST_CASE_DIR 설정

TEST_CASE_DIR=$1


# 디렉토리 이름 추출

BASE_NAME=$(basename "$TEST_CASE_DIR")

# 출력 헤더 파일 이름 설정


OUTPUT_HEADER="test_cases_${BASE_NAME}.h"
# 헤더 파일 초기화

echo "#ifndef TEST_CASES_H" > $OUTPUT_HEADER
echo "#define TEST_CASES_H" >> $OUTPUT_HEADER
echo "" >> $OUTPUT_HEADER

# 각 .c 파일에서 함수 이름 추출

for file in $(find "$TEST_CASE_DIR" -name "*.c"); do
	    grep -E "void CWE[0-9]+" "$file" | awk '{print $2}' | sed 's/(.*//g' >> temp.txt
done

# 함수 선언을 헤더 파일에 추가
awk '{print "void " $1 "();"}' temp.txt >> $OUTPUT_HEADER
echo "" >> $OUTPUT_HEADER
echo "#endif" >> $OUTPUT_HEADER


# 임시 파일 삭제

rm temp.txt

echo "Header file $OUTPUT_HEADER generated!"
