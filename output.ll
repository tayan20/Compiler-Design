; ModuleID = 'mini-c'
source_filename = "mini-c"

define float @test() {
entry:
  %x = alloca i32, align 4
  store i32 0, ptr %x, align 4
  store i32 5, ptr %x, align 4
  %x1 = load i32, ptr %x, align 4
  %intToFloat = sitofp i32 %x1 to float
  ret float %intToFloat
}
