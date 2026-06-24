class Point {
    public:
    int x;
    int y;
};

int main() {
    Point p;
    p.x = 10;
    p.y = 20;
    Point* ptr;
    ptr = &p;
    int a;
    a = ptr->x;
    int b;
    b = ptr->y;
    Point* p2;
    p2 = new Point();
    p2->x = 30;
    return a;
}
