; ModuleID = 'mini-c'
source_filename = "mini-c"

declare i32 @print_int(i32)

define i32 @vector_total(ptr %a, ptr %b, i32 %n) {
entry:
  %total = alloca i32, align 4
  %i = alloca i32, align 4
  %n3 = alloca i32, align 4
  %b2 = alloca ptr, align 8
  %a1 = alloca ptr, align 8
  store ptr %a, ptr %a1, align 8
  store ptr %b, ptr %b2, align 8
  store i32 %n, ptr %n3, align 4
  store i32 0, ptr %i, align 4
  store i32 0, ptr %total, align 4
  store i32 0, ptr %i, align 4
  br label %loopcond

loopcond:                                         ; preds = %loopbody, %entry
  %i4 = load i32, ptr %i, align 4
  %n5 = load i32, ptr %n3, align 4
  %cmptmp = icmp slt i32 %i4, %n5
  br i1 %cmptmp, label %loopbody, label %afterloop

loopbody:                                         ; preds = %loopcond
  %total6 = load i32, ptr %total, align 4
  %a_ptr = load ptr, ptr %a1, align 8
  %i7 = load i32, ptr %i, align 4
  %arrayidx = getelementptr i32, ptr %a_ptr, i32 %i7
  %a_elem = load i32, ptr %arrayidx, align 4
  %addtmp = add i32 %total6, %a_elem
  %b_ptr = load ptr, ptr %b2, align 8
  %i8 = load i32, ptr %i, align 4
  %arrayidx9 = getelementptr i32, ptr %b_ptr, i32 %i8
  %b_elem = load i32, ptr %arrayidx9, align 4
  %addtmp10 = add i32 %addtmp, %b_elem
  store i32 %addtmp10, ptr %total, align 4
  %i11 = load i32, ptr %i, align 4
  %addtmp12 = add i32 %i11, 1
  store i32 %addtmp12, ptr %i, align 4
  br label %loopcond

afterloop:                                        ; preds = %loopcond
  %total13 = load i32, ptr %total, align 4
  ret i32 %total13
}
