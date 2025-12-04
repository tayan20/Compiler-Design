; ModuleID = 'mini-c'
source_filename = "mini-c"

declare i32 @print_int(i32)

define i32 @test_array_assignment() {
entry:
  %x = alloca i32, align 4
  %arr = alloca [5 x i32], align 4
  call void @llvm.memset.p0.i64(ptr align 4 %arr, i8 0, i64 20, i1 false)
  store i32 0, ptr %x, align 4
  %arrayidx = getelementptr [5 x i32], ptr %arr, i32 0, i32 0
  store i32 10, ptr %arrayidx, align 4
  %arrayidx1 = getelementptr [5 x i32], ptr %arr, i32 0, i32 1
  store i32 20, ptr %arrayidx1, align 4
  %arrayidx2 = getelementptr [5 x i32], ptr %arr, i32 0, i32 0
  %arr_elem = load i32, ptr %arrayidx2, align 4
  %arrayidx3 = getelementptr [5 x i32], ptr %arr, i32 0, i32 1
  %arr_elem4 = load i32, ptr %arrayidx3, align 4
  %addtmp = add i32 %arr_elem, %arr_elem4
  %arrayidx5 = getelementptr [5 x i32], ptr %arr, i32 0, i32 2
  store i32 %addtmp, ptr %arrayidx5, align 4
  %arrayidx6 = getelementptr [5 x i32], ptr %arr, i32 0, i32 2
  %arr_elem7 = load i32, ptr %arrayidx6, align 4
  store i32 %arr_elem7, ptr %x, align 4
  %x8 = load i32, ptr %x, align 4
  %calltmp = call i32 @print_int(i32 %x8)
  %x9 = load i32, ptr %x, align 4
  ret i32 %x9
}

; Function Attrs: nocallback nofree nounwind willreturn memory(argmem: write)
declare void @llvm.memset.p0.i64(ptr writeonly captures(none), i8, i64, i1 immarg) #0

attributes #0 = { nocallback nofree nounwind willreturn memory(argmem: write) }
