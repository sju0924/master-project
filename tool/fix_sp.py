def move_app_blocks(input_file, output_file):
    # 파일 읽기
    with open(input_file, 'r') as file:
        lines = file.readlines()

    i = 0
    blocks = []
    # @APP
	# sub	sp, #72
	# @NO_APP

    # 블록 탐색: @APP, sub sp, #72, @NO_APP
    while i < len(lines) - 2:  # 최소 3줄은 있어야 블록으로 간주
        if "@APP" in lines[i] and "sub	sp, #72" in lines[i + 1] and "@NO_APP" in lines[i + 2]:
    
            blocks.append((i, i + 2))  # 블록 시작과 끝 줄 번호 저장
            i += 3  # 블록의 끝까지 건너뛰기
        elif "@APP" in lines[i] and "add	sp, #72" in lines[i + 1] and "@NO_APP" in lines[i + 2]:
       
            blocks.append((i, i + 2))  # 블록 시작과 끝 줄 번호 저장
            i += 3  # 블록의 끝까지 건너뛰기
        else:
            i += 1
            
        if ".file" in lines [i]:
            lines[i] = lines[i].replace('" "','')

    # 각 블록을 가장 가까운 bl 명령어 위로 이동
    for block_start, block_end in reversed(blocks):  # 뒤에서부터 처리
        app_block = lines[block_start:block_end + 1]  # 블록 추출
        del lines[block_start:block_end + 1]  # 기존 위치에서 블록 삭제

        # bl 명령어 찾기
        if "sub" in app_block[1]:
            for i in range(block_start, len(lines)):
                if "bl	" in lines[i]:  # bl 명령어 발견
                    if "configure_mpu_redzone_for_call" not in lines[i]:
                        print("insert line sub", i, ", inst ",lines[i],end="")
                    lines = lines[:i] + app_block + lines[i:]  # bl 명령어 바로 위에 삽입
                    break
        else:
            for i in range(block_start, 0, -1):
                if "bl	" in lines[i]:  # bl 명령어 발견
                    if "configure_mpu_redzone_for_call" not in lines[i]:
                        print("insert line add", i, ", inst ",lines[i],end="")
                    lines = lines[:i+1] + app_block + lines[i+1:]  # bl 명령어 바로 위에 삽입
                    break
            

    # 결과 저장
    with open(output_file, 'w') as file:
        file.writelines(lines)

# 실행
input_file = 'output.s'  # 원본 파일
output_file = 'fixed_output.s'  # 수정된 파일

move_app_blocks(input_file, output_file)
