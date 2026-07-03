class Point {
public:
    int x;
    int y;
};

int main() {
    Point p;
    p.x = 10;
    p.y = 20;
    Point* ptr = &p;
    ptr->x = 30;
    return 0;
}
