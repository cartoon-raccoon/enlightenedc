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

// --- Accessing an inherited member through a pointer to the derived class ---

Rectangle *rp = &rect;
"%d\n", rp->x;       // inherited from Shape
"%d\n", rp->width;   // own member

// Implicit upcast from a derived-class pointer to a base-class pointer at a
// call site, mirroring ClassType::coercible_to's upcast rule at the value
// level (PointerType::coercible_to in src/semantics/types.cpp).
I32 area_via_base(Shape *s) {
    return s->x + s->y;
}

"%d\n", area_via_base(rp);
