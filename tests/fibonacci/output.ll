; ModuleID = 'mini-c'
source_filename = "mini-c"

declare i32 @print_int(i32)

define i32 @fibonacci(i32 %n) {
entry:
  %total = alloca i32, align 4
  %c = alloca i32, align 4
  %next = alloca i32, align 4
  %second = alloca i32, align 4
  %first = alloca i32, align 4
  %n1 = alloca i32, align 4
  store i32 %n, ptr %n1, align 4
  store i32 0, ptr %first, align 4
  store i32 0, ptr %second, align 4
  store i32 0, ptr %next, align 4
  store i32 0, ptr %c, align 4
  store i32 0, ptr %total, align 4
  %n2 = load i32, ptr %n1, align 4
  %calltmp = call i32 @print_int(i32 %n2)
  store i32 0, ptr %first, align 4
  store i32 1, ptr %second, align 4
  store i32 1, ptr %c, align 4
  store i32 0, ptr %total, align 4
  br label %loopcond

loopcond:                                         ; preds = %ifcont, %entry
  %c3 = load i32, ptr %c, align 4
  %n4 = load i32, ptr %n1, align 4
  %cmptmp = icmp slt i32 %c3, %n4
  br i1 %cmptmp, label %loopbody, label %afterloop

loopbody:                                         ; preds = %loopcond
  %c5 = load i32, ptr %c, align 4
  %cmptmp6 = icmp sle i32 %c5, 1
  br i1 %cmptmp6, label %then, label %else

then:                                             ; preds = %loopbody
  %c7 = load i32, ptr %c, align 4
  store i32 %c7, ptr %next, align 4
  br label %ifcont

else:                                             ; preds = %loopbody
  %first8 = load i32, ptr %first, align 4
  %second9 = load i32, ptr %second, align 4
  %addtmp = add i32 %first8, %second9
  store i32 %addtmp, ptr %next, align 4
  %second10 = load i32, ptr %second, align 4
  store i32 %second10, ptr %first, align 4
  %next11 = load i32, ptr %next, align 4
  store i32 %next11, ptr %second, align 4
  br label %ifcont

ifcont:                                           ; preds = %else, %then
  %next12 = load i32, ptr %next, align 4
  %calltmp13 = call i32 @print_int(i32 %next12)
  %c14 = load i32, ptr %c, align 4
  %addtmp15 = add i32 %c14, 1
  store i32 %addtmp15, ptr %c, align 4
  %total16 = load i32, ptr %total, align 4
  %next17 = load i32, ptr %next, align 4
  %addtmp18 = add i32 %total16, %next17
  store i32 %addtmp18, ptr %total, align 4
  br label %loopcond

afterloop:                                        ; preds = %loopcond
  %total19 = load i32, ptr %total, align 4
  %calltmp20 = call i32 @print_int(i32 %total19)
  %total21 = load i32, ptr %total, align 4
  ret i32 %total21
}
