# 03-const用法总结

## 修饰变量

```cpp
const int a = 10;      // a 不可修改
int const a = 10;      // 等价写法
```

## 修饰指针

```cpp
const int *p;          // 指向的值不可改
int *const p;          // 指针本身不可改
const int *const p;    // 都不可改
```

技巧：看 const 在 * 的哪一边

## 修饰成员函数

```cpp
class A {
    int getValue() const { return val; }  // 不修改成员变量
};
```