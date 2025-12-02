; ModuleID = 'mini-c'
source_filename = "mini-c"

declare i32 @print_int(i32)

declare float @print_float(float)

define float @unary(i32 %n, float %m) {
entry:
  %sum = alloca float, align 4
  %result = alloca float, align 4
  %m2 = alloca float, align 4
  %n1 = alloca i32, align 4
  store i32 %n, ptr %n1, align 4
  store float %m, ptr %m2, align 4
  store float 0.000000e+00, ptr %result, align 4
  store float 0.000000e+00, ptr %sum, align 4
  store float 0.000000e+00, ptr %sum, align 4
  %n3 = load i32, ptr %n1, align 4
  %m4 = load float, ptr %m2, align 4
  %intToFloat = sitofp i32 %n3 to float
  %addtmp = fadd float %intToFloat, %m4
  store float %addtmp, ptr %result, align 4
  %result5 = load float, ptr %result, align 4
  %calltmp = call float @print_float(float %result5)
  %sum6 = load float, ptr %sum, align 4
  %result7 = load float, ptr %result, align 4
  %addtmp8 = fadd float %sum6, %result7
  store float %addtmp8, ptr %sum, align 4
  %n9 = load i32, ptr %n1, align 4
  %m10 = load float, ptr %m2, align 4
  %negtmp = fneg float %m10
  %intToFloat11 = sitofp i32 %n9 to float
  %addtmp12 = fadd float %intToFloat11, %negtmp
  store float %addtmp12, ptr %result, align 4
  %result13 = load float, ptr %result, align 4
  %calltmp14 = call float @print_float(float %result13)
  %sum15 = load float, ptr %sum, align 4
  %result16 = load float, ptr %result, align 4
  %addtmp17 = fadd float %sum15, %result16
  store float %addtmp17, ptr %sum, align 4
  %n18 = load i32, ptr %n1, align 4
  %m19 = load float, ptr %m2, align 4
  %negtmp20 = fneg float %m19
  %negtmp21 = fneg float %negtmp20
  %intToFloat22 = sitofp i32 %n18 to float
  %addtmp23 = fadd float %intToFloat22, %negtmp21
  store float %addtmp23, ptr %result, align 4
  %result24 = load float, ptr %result, align 4
  %calltmp25 = call float @print_float(float %result24)
  %sum26 = load float, ptr %sum, align 4
  %result27 = load float, ptr %result, align 4
  %addtmp28 = fadd float %sum26, %result27
  store float %addtmp28, ptr %sum, align 4
  %n29 = load i32, ptr %n1, align 4
  %negtmp30 = sub i32 0, %n29
  %m31 = load float, ptr %m2, align 4
  %negtmp32 = fneg float %m31
  %intToFloat33 = sitofp i32 %negtmp30 to float
  %addtmp34 = fadd float %intToFloat33, %negtmp32
  store float %addtmp34, ptr %result, align 4
  %result35 = load float, ptr %result, align 4
  %calltmp36 = call float @print_float(float %result35)
  %sum37 = load float, ptr %sum, align 4
  %result38 = load float, ptr %result, align 4
  %addtmp39 = fadd float %sum37, %result38
  store float %addtmp39, ptr %sum, align 4
  %sum40 = load float, ptr %sum, align 4
  ret float %sum40
}
