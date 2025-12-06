; ModuleID = 'mini-c'
source_filename = "mini-c"

@a = common global [10 x i32] zeroinitializer
@b = common global [10 x [10 x i32]] zeroinitializer

declare i32 @print_int(i32)

define i32 @init_arrays() {
entry:
  %j = alloca i32, align 4
  %i = alloca i32, align 4
  store i32 0, ptr %i, align 4
  store i32 0, ptr %j, align 4
  store i32 0, ptr %i, align 4
  store i32 0, ptr %j, align 4
  br label %loopcond

loopcond:                                         ; preds = %afterloop, %entry
  %i1 = load i32, ptr %i, align 4
  %cmptmp = icmp slt i32 %i1, 10
  br i1 %cmptmp, label %loopbody, label %afterloop19

loopbody:                                         ; preds = %loopcond
  store i32 0, ptr %j, align 4
  br label %loopcond2

loopcond2:                                        ; preds = %loopbody5, %loopbody
  %j3 = load i32, ptr %j, align 4
  %cmptmp4 = icmp slt i32 %j3, 10
  br i1 %cmptmp4, label %loopbody5, label %afterloop

loopbody5:                                        ; preds = %loopcond2
  %i6 = load i32, ptr %i, align 4
  %addtmp = add i32 %i6, 1
  %j7 = load i32, ptr %j, align 4
  %addtmp8 = add i32 %j7, 1
  %multmp = mul i32 %addtmp, %addtmp8
  %i9 = load i32, ptr %i, align 4
  %j10 = load i32, ptr %j, align 4
  %arrayidx = getelementptr [10 x [10 x i32]], ptr @b, i32 0, i32 %i9, i32 %j10
  store i32 %multmp, ptr %arrayidx, align 4
  %j11 = load i32, ptr %j, align 4
  %addtmp12 = add i32 %j11, 1
  store i32 %addtmp12, ptr %j, align 4
  br label %loopcond2

afterloop:                                        ; preds = %loopcond2
  %i13 = load i32, ptr %i, align 4
  %addtmp14 = add i32 %i13, 1
  %i15 = load i32, ptr %i, align 4
  %arrayidx16 = getelementptr [10 x i32], ptr @a, i32 0, i32 %i15
  store i32 %addtmp14, ptr %arrayidx16, align 4
  %i17 = load i32, ptr %i, align 4
  %addtmp18 = add i32 %i17, 1
  store i32 %addtmp18, ptr %i, align 4
  br label %loopcond

afterloop19:                                      ; preds = %loopcond
  ret i32 0
}

define i32 @vector_weighted_total() {
entry:
  %j = alloca i32, align 4
  %i = alloca i32, align 4
  %total = alloca i32, align 4
  store i32 0, ptr %total, align 4
  store i32 0, ptr %i, align 4
  store i32 0, ptr %j, align 4
  %calltmp = call i32 @init_arrays()
  store i32 0, ptr %i, align 4
  store i32 0, ptr %j, align 4
  store i32 0, ptr %total, align 4
  br label %loopcond

loopcond:                                         ; preds = %afterloop, %entry
  %i1 = load i32, ptr %i, align 4
  %cmptmp = icmp slt i32 %i1, 10
  br i1 %cmptmp, label %loopbody, label %afterloop15

loopbody:                                         ; preds = %loopcond
  store i32 0, ptr %j, align 4
  br label %loopcond2

loopcond2:                                        ; preds = %loopbody5, %loopbody
  %j3 = load i32, ptr %j, align 4
  %cmptmp4 = icmp slt i32 %j3, 10
  br i1 %cmptmp4, label %loopbody5, label %afterloop

loopbody5:                                        ; preds = %loopcond2
  %total6 = load i32, ptr %total, align 4
  %i7 = load i32, ptr %i, align 4
  %arrayidx = getelementptr [10 x i32], ptr @a, i32 0, i32 %i7
  %a_elem = load i32, ptr %arrayidx, align 4
  %i8 = load i32, ptr %i, align 4
  %j9 = load i32, ptr %j, align 4
  %arrayidx10 = getelementptr [10 x [10 x i32]], ptr @b, i32 0, i32 %i8, i32 %j9
  %b_elem = load i32, ptr %arrayidx10, align 4
  %multmp = mul i32 %a_elem, %b_elem
  %addtmp = add i32 %total6, %multmp
  store i32 %addtmp, ptr %total, align 4
  %j11 = load i32, ptr %j, align 4
  %addtmp12 = add i32 %j11, 1
  store i32 %addtmp12, ptr %j, align 4
  br label %loopcond2

afterloop:                                        ; preds = %loopcond2
  %i13 = load i32, ptr %i, align 4
  %addtmp14 = add i32 %i13, 1
  store i32 %addtmp14, ptr %i, align 4
  br label %loopcond

afterloop15:                                      ; preds = %loopcond
  %total16 = load i32, ptr %total, align 4
  ret i32 %total16
}
