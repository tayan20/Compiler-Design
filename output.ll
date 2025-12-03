; ModuleID = 'mini-c'
source_filename = "mini-c"

@globalArr = common global [10 x i32] zeroinitializer
@globalMatrix = common global [4 x [4 x float]] zeroinitializer
@global3D = common global [2 x [3 x [4 x i32]]] zeroinitializer

declare i32 @print_int(i32)

define i32 @arr_global_decl() {
entry:
  %x = alloca i32, align 4
  store i32 0, ptr %x, align 4
  store i32 10, ptr %x, align 4
  %x1 = load i32, ptr %x, align 4
  %calltmp = call i32 @print_int(i32 %x1)
  %x2 = load i32, ptr %x, align 4
  ret i32 %x2
}
