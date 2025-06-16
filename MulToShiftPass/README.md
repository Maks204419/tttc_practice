# MulToShiftPass

## Описание
LLVM-проход, который заменяет инструкции `mul` на `shl`, если множитель — степень двойки.

## Компиляция
```bash
clang++ -shared -fPIC `llvm-config --cxxflags` MulToShiftPass.cpp -o MulToShiftPass.so `llvm-config --ldflags`
```

## Применение
```bash
opt -load-pass-plugin ./MulToShiftPass.so -passes="mul-to-shift" -S input.ll -o output.ll
```

## Пример
**input.ll**
```llvm
define i32 @test(i32 %a) {
  %b = mul i32 %a, 8
  ret i32 %b
}
```

**output.ll**
```llvm
define i32 @test(i32 %a) {
  %1 = shl i32 %a, 3
  ret i32 %1
}
```
