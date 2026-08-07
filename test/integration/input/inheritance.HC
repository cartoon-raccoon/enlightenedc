// Single-level inheritance: Rectangle extends Shape
class Shape {
    I32 x;
    I32 y;
};

class Rectangle : Shape {
    I32 width;
    I32 height;
};

// Two-level inheritance: ColoredRect extends Rectangle extends Shape
class ColoredRect : Rectangle {
    I32 color;
};

// --- Single-level: access parent and own members ---

class Rectangle rect;
rect.x = 10;
rect.y = 20;
rect.width = 100;
rect.height = 50;

"%d\n", rect.x;
"%d\n", rect.y;
"%d\n", rect.width;
"%d\n", rect.height;

// --- Two-level: access grandparent, parent, and own members ---

class ColoredRect crect;
crect.x = 1;
crect.y = 2;
crect.width = 3;
crect.height = 4;
crect.color = 255;

"%d\n", crect.x;
"%d\n", crect.y;
"%d\n", crect.width;
"%d\n", crect.height;
"%d\n", crect.color;
