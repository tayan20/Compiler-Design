; ModuleID = 'mini-c'
source_filename = "mini-c"

declare i32 @print_int(i32)

define i32 @arr_addition(i32 %n, i32 %m) {
entry:
  %temp = alloca i32, align 4
  %idx = alloca i32, align 4
  %result = alloca i32, align 4
  %arr_result = alloca [10 x [5 x i32]], align 4
  %m2 = alloca i32, align 4
  %n1 = alloca i32, align 4
  store i32 %n, ptr %n1, align 4
  store i32 %m, ptr %m2, align 4
  call void @llvm.memset.p0.i64(ptr align 4 %arr_result, i8 0, i64 200, i1 false)
  store i32 0, ptr %result, align 4
  store i32 0, ptr %idx, align 4
  store i32 0, ptr %temp, align 4
  %m3 = load i32, ptr %m2, align 4
  %arrayidx = getelementptr [10 x [5 x i32]], ptr %arr_result, i32 0, i32 0, i32 0
  store i32 %m3, ptr %arrayidx, align 4
  %n4 = load i32, ptr %n1, align 4
  %arrayidx5 = getelementptr [10 x [5 x i32]], ptr %arr_result, i32 0, i32 0, i32 1
  store i32 %n4, ptr %arrayidx5, align 4
  %arrayidx6 = getelementptr [10 x [5 x i32]], ptr %arr_result, i32 0, i32 0, i32 0
  %arr_result_elem = load i32, ptr %arrayidx6, align 4
  %arrayidx7 = getelementptr [10 x [5 x i32]], ptr %arr_result, i32 0, i32 0, i32 1
  %arr_result_elem8 = load i32, ptr %arrayidx7, align 4
  %addtmp = add i32 %arr_result_elem, %arr_result_elem8
  store i32 %addtmp, ptr %temp, align 4
  %temp9 = load i32, ptr %temp, align 4
  ret i32 %temp9
}

; Function Attrs: nocallback nofree nounwind willreturn memory(argmem: write)
declare void @llvm.memset.p0.i64(ptr writeonly captures(none), i8, i64, i1 immarg) #0

attributes #0 = { nocallback nofree nounwind willreturn memory(argmem: write) }
