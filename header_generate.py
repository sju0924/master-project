import os
import re
import sys

def generate_header(test_case_dir):
    # 디렉토리 이름 확인
    if not os.path.exists(test_case_dir):
        print(f"Error: Directory '{test_case_dir}' does not exist.")
        sys.exit(1)

    # 디렉토리 이름 추출
    base_name = os.path.basename(test_case_dir)
    output_header = f"test_cases_{base_name}.h"

    # 헤더 파일 초기화
    with open(output_header, 'w') as header_file:
        header_file.write("#ifndef TEST_CASES_H\n")
        header_file.write("#define TEST_CASES_H\n\n")

    # .c 파일에서 함수 선언 추출
    functions = []
    for root, _, files in os.walk(test_case_dir):
        for file in files:
            if file.endswith(".c"):
                file_path = os.path.join(root, file)
                with open(file_path, 'r') as c_file:
                    functions.append('\n')
                    for line in c_file:
                        match = re.match(r"^\s*void\s+(CWE[0-9]+[a-zA-Z0-9_]*)\s*\(", line)
                        if match:
                            functions.append(match.group(1))

    # 함수 선언을 헤더 파일에 추가
    with open(output_header, 'a') as header_file:
        for func in functions:
            if func == '\n':
                header_file.write(func)
            else:
                header_file.write(f"void {func}();\n")
        header_file.write("\n#endif\n")

    print(f"Header file {output_header} generated!")

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: python script.py <TEST_CASE_DIR>")
        sys.exit(1)

    test_case_dir = sys.argv[1]
    generate_header(test_case_dir)
