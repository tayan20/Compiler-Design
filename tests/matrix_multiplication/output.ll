; ModuleID = 'mini-c'
source_filename = "mini-c"

declare float @print_float(float)

define i32 @matrix_mul(ptr %a, ptr %b, ptr %c, i32 %n) {
entry:
  %k = alloca i32, align 4
  %j = alloca i32, align 4
  %i = alloca i32, align 4
  %n4 = alloca i32, align 4
  %c3 = alloca ptr, align 8
  %b2 = alloca ptr, align 8
  %a1 = alloca ptr, align 8
  store ptr %a, ptr %a1, align 8
  store ptr %b, ptr %b2, align 8
  store ptr %c, ptr %c3, align 8
  store i32 %n, ptr %n4, align 4
  store i32 0, ptr %i, align 4
  store i32 0, ptr %j, align 4
  store i32 0, ptr %k, align 4
  store i32 0, ptr %i, align 4
  br label %loopcond

loopcond:                                         ; preds = %afterloop39, %entry
  %i5 = load i32, ptr %i, align 4
  %n6 = load i32, ptr %n4, align 4
  %cmptmp = icmp slt i32 %i5, %n6
  br i1 %cmptmp, label %loopbody, label %afterloop42

loopbody:                                         ; preds = %loopcond
  store i32 0, ptr %j, align 4
  br label %loopcond7

loopcond7:                                        ; preds = %afterloop, %loopbody
  %j8 = load i32, ptr %j, align 4
  %n9 = load i32, ptr %n4, align 4
  %cmptmp10 = icmp slt i32 %j8, %n9
  br i1 %cmptmp10, label %loopbody11, label %afterloop39

loopbody11:                                       ; preds = %loopcond7
  store i32 0, ptr %k, align 4
  br label %loopcond12

loopcond12:                                       ; preds = %loopbody16, %loopbody11
  %k13 = load i32, ptr %k, align 4
  %n14 = load i32, ptr %n4, align 4
  %cmptmp15 = icmp slt i32 %k13, %n14
  br i1 %cmptmp15, label %loopbody16, label %afterloop

loopbody16:                                       ; preds = %loopcond12
  %c_ptr = load ptr, ptr %c3, align 8
  %i17 = load i32, ptr %i, align 4
  %j18 = load i32, ptr %j, align 4
  %offset_mul = mul i32 %i17, 10
  %offset_add = add i32 %offset_mul, %j18
  %arrayidx = getelementptr float, ptr %c_ptr, i32 %offset_add
  %c_elem = load float, ptr %arrayidx, align 4
  %a_ptr = load ptr, ptr %a1, align 8
  %i19 = load i32, ptr %i, align 4
  %k20 = load i32, ptr %k, align 4
  %offset_mul21 = mul i32 %i19, 10
  %offset_add22 = add i32 %offset_mul21, %k20
  %arrayidx23 = getelementptr float, ptr %a_ptr, i32 %offset_add22
  %a_elem = load float, ptr %arrayidx23, align 4
  %b_ptr = load ptr, ptr %b2, align 8
  %k24 = load i32, ptr %k, align 4
  %j25 = load i32, ptr %j, align 4
  %offset_mul26 = mul i32 %k24, 10
  %offset_add27 = add i32 %offset_mul26, %j25
  %arrayidx28 = getelementptr float, ptr %b_ptr, i32 %offset_add27
  %b_elem = load float, ptr %arrayidx28, align 4
  %multmp = fmul float %a_elem, %b_elem
  %addtmp = fadd float %c_elem, %multmp
  %c_ptr29 = load ptr, ptr %c3, align 8
  %i30 = load i32, ptr %i, align 4
  %j31 = load i32, ptr %j, align 4
  %offset_mul32 = mul i32 %i30, 10
  %offset_add33 = add i32 %offset_mul32, %j31
  %arrayidx34 = getelementptr float, ptr %c_ptr29, i32 %offset_add33
  store float %addtmp, ptr %arrayidx34, align 4
  %k35 = load i32, ptr %k, align 4
  %addtmp36 = add i32 %k35, 1
  store i32 %addtmp36, ptr %k, align 4
  br label %loopcond12

afterloop:                                        ; preds = %loopcond12
  %j37 = load i32, ptr %j, align 4
  %addtmp38 = add i32 %j37, 1
  store i32 %addtmp38, ptr %j, align 4
  br label %loopcond7

afterloop39:                                      ; preds = %loopcond7
  %i40 = load i32, ptr %i, align 4
  %addtmp41 = add i32 %i40, 1
  store i32 %addtmp41, ptr %i, align 4
  br label %loopcond

afterloop42:                                      ; preds = %loopcond
  ret i32 0
}
