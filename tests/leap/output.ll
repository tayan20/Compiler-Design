; ModuleID = 'mini-c'
source_filename = "mini-c"

declare i32 @print_int(i32)

define i32 @is_leap(i32 %year) {
entry:
  %temp = alloca i32, align 4
  %isLeap = alloca i32, align 4
  %year1 = alloca i32, align 4
  store i32 %year, ptr %year1, align 4
  store i32 0, ptr %isLeap, align 4
  store i32 0, ptr %temp, align 4
  %year2 = load i32, ptr %year1, align 4
  %modtmp = srem i32 %year2, 4
  %cmptmp = icmp eq i32 %modtmp, 0
  br i1 %cmptmp, label %then, label %else13

then:                                             ; preds = %entry
  %year3 = load i32, ptr %year1, align 4
  %modtmp4 = srem i32 %year3, 100
  %cmptmp5 = icmp eq i32 %modtmp4, 0
  br i1 %cmptmp5, label %then6, label %else11

then6:                                            ; preds = %then
  %year7 = load i32, ptr %year1, align 4
  %modtmp8 = srem i32 %year7, 400
  %cmptmp9 = icmp eq i32 %modtmp8, 0
  br i1 %cmptmp9, label %then10, label %else

then10:                                           ; preds = %then6
  store i32 1, ptr %isLeap, align 4
  br label %ifcont

else:                                             ; preds = %then6
  store i32 0, ptr %isLeap, align 4
  br label %ifcont

ifcont:                                           ; preds = %else, %then10
  br label %ifcont12

else11:                                           ; preds = %then
  store i32 1, ptr %isLeap, align 4
  br label %ifcont12

ifcont12:                                         ; preds = %else11, %ifcont
  br label %ifcont14

else13:                                           ; preds = %entry
  store i32 0, ptr %isLeap, align 4
  br label %ifcont14

ifcont14:                                         ; preds = %else13, %ifcont12
  %isLeap15 = load i32, ptr %isLeap, align 4
  ret i32 %isLeap15
}
