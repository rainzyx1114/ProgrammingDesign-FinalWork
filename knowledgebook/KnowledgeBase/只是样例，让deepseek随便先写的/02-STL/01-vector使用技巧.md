# 01-vector使用技巧

## 预分配空间

```cpp
vector<int> v;
v.reserve(1000);  // 避免多次扩容
```

## 删除元素

```cpp
// 删除特定值（erase-remove 惯用法）
v.erase(remove(v.begin(), v.end(), 42), v.end());

// 按条件删除
v.erase(remove_if(v.begin(), v.end(), [](int x) {
    return x % 2 == 0;
}), v.end());
```

## 遍历

```cpp
for (const auto &item : v) {  // 用引用避免拷贝
    cout << item << endl;
}
```