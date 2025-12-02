; ModuleID = 'mini-c'
source_filename = "mini-c"

declare i32 @print_int(i32)

define void @Void() {
entry:
  %result = alloca i32, align 4
  store i32 0, ptr %result, align 4
  store i32 0, ptr %result, align 4
  %result1 = load i32, ptr %result, align 4
  %calltmp = call i32 @print_int(i32 %result1)
  br label %loopcond

loopcond:                                         ; preds = %loopbody, %entry
  %result2 = load i32, ptr %result, align 4
  %cmptmp = icmp slt i32 %result2, 10
  br i1 %cmptmp, label %loopbody, label %afterloop

loopbody:                                         ; preds = %loopcond
  %result3 = load i32, ptr %result, align 4
  %addtmp = add i32 %result3, 1
  store i32 %addtmp, ptr %result, align 4
  %result4 = load i32, ptr %result, align 4
  %calltmp5 = call i32 @print_int(i32 %result4)
  br label %loopcond

afterloop:                                        ; preds = %loopcond
  ret void
}
