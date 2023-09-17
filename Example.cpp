#include <exception>
#include <vector>
#include <algorithm>
#include <math.h>

#include "draw_pixel.h"


struct Point {
    int x, y;

    bool operator==(const Point& other) const {
        return x == other.x && y == other.y;
    }
    bool operator!=(const Point& other) const {
        return !(*this == other);
    }
};


struct Segment {
    Point Point1, Point2;
};


class Color {
    const int COLOR_MIN_VALUE = 0;
    const int COLOR_MAX_VALUE = 255;

    public:
        Color(int r, int g, int b) : R(r), G(g), B(b) {
            if (!CheckColorValue(r) || !CheckColorValue(g) || !CheckColorValue(b)) {
                throw std::exception("The color is not valid");
            }
        }

        std::tuple<int, int, int> GetTuple() const {
            return std::make_tuple(R, G, B);
        }
    
    private:
        bool CheckColorValue(int value) const {
            return value >= COLOR_MIN_VALUE && value <= COLOR_MAX_VALUE;
        }

        int R, G, B;
};


class Shape {
    public:
        Shape(Color fillColor, Color borderColor, int borderWidth, int displayWidth, int displayHeight) :
            FillColor(fillColor), BorderColor(borderColor), BorderWidth(borderWidth), DisplayWidth(displayWidth), DisplayHeight(displayHeight) {}
        const virtual void Draw() = 0;
    
    protected:
        Color FillColor;
        Color BorderColor;

        int BorderWidth;
        int DisplayWidth;
        int DisplayHeight;

        // Область нахождения фигуры
        int xStart;
        int xEnd;
        int yStart;
        int yEnd;
};


class Circle : public Shape {
    public:
        Circle(Color fillColor, Color borderColor, int borderWidth, int displayWidth, int displayHeight, Point center, int radius) : 
            Shape(fillColor, borderColor, borderWidth, displayWidth, displayHeight), Center(center), Radius(radius) {
                xStart = std::max(Center.x - Radius, 0);
                xEnd = std::min(Center.x + Radius, DisplayWidth - 1);
                yStart = std::max(Center.y - Radius, 0);
                yEnd = std::min(Center.y + Radius, DisplayHeight);
            }
        
        void Draw() const {
            for (int x = xStart; x <= xEnd; ++x) {
                for (int y = yStart; y <= yEnd; ++y) {
                    if (x * x + y * y <= Radius * Radius) {
                        if (x * x + y * y <= (Radius - BorderWidth) * (Radius - BorderWidth)) {
                            DrawPixel(x, y, FillColor.GetTuple());
                        } else {
                            DrawPixel(x, y, BorderColor.GetTuple());
                        }
                    }
                }
            }
        }

    private:
        Point Center;
        int Radius;
};


class line {
    public:
        line(Point a, Point b) {
            A = a.y - b.y;
            B = b.x - a.x;
            C = -a.x * A - a.y * B;
        }
        line(int a, int b, int c) : A(a), B(b), C(c) {}

        bool operator==(const line& other) const {
            return A == other.A && B == other.B && C == other.C;
        }

        int A;
        int B;
        int C;
};


class Polygon : public Shape {
    public:
        Polygon(Color fillColor, Color borderColor, int borderWidth, int displayWidth, int displayHeight, const std::vector<Segment>& edges) : 
            Shape(fillColor, borderColor, borderWidth, displayWidth, displayHeight), Edges(edges) {
                xStart = displayWidth;
                xEnd = 0;
                yStart = displayHeight;
                yEnd = 0;
                
                for (int i = 0; i < edges.size(); ++i) {
                    // Проверка, что многоугольник задан корректно, в обходе по часовой стрелке
                    if (edges[i].Point2 != edges[(i + 1) % edges.size()].Point1) { 
                        throw std::exception("The polygon is not valid");
                    }
                    
                    xStart = std::min(xStart, edges[i].Point1.x);
                    xEnd = std::max(xEnd, edges[i].Point1.x);
                    yStart = std::min(yStart, edges[i].Point1.y);
                    yEnd = std::max(yEnd, edges[i].Point1.y);
                }

                xStart = std::max(xStart, 0);
                xEnd = std::min(xEnd, displayWidth - 1);
                yStart = std::max(yStart, 0);
                yEnd = std::min(yEnd, displayHeight - 1);
            }
        
        void Draw() const {
             for (int x = xStart; x <= xEnd; ++x) {
                for (int y = yStart; y <= yEnd; ++y) {
                    if (InPolygon(x, y)) {
                        if (IsBorder(x, y)) {
                            DrawPixel(x, y, BorderColor.GetTuple());
                        } else {
                            DrawPixel(x, y, FillColor.GetTuple());
                        }
                    }
                }
             }
        }

    private:
        std::vector<Segment> Edges;
        
        double Det(int a, int b, int c, int d) const {
            return a * d - b * c;
        }

        bool Intersect(int x, int y, line line1, Point Point1, Point Point2) const {
            auto line2 = line(Point1, Point2);

            if (line1 == line2) { // Если луч параллелен ребру, пересечение не засчитываем
                return false;
            }
            
            double zn = Det (line1.A, line1.B, line2.A, line2.B);
            if (abs(zn) < 0) {
                return false;
            }
            
            // Находим точку пересечения
            Point res;
            res.x = -Det(line1.C, line1.B, line2.C, line2.B) / zn;
            res.y = -Det(line1.A, line1.C, line2.A, line2.C) / zn;
            
            // Проверяем, что точка пересечения лежит на отрезке
            if (!(res.x >= Point1.x && res.x <= Point2.x && res.y >= Point1.y && res.y <= Point2.y)) { 
                return false;
            }

            // Проверяем, что точка пересечения по нужную сторону от луча
            if (!((res.x > x) == (Point2.x > x) && (res.y > y) == (Point2.y > y))) {
                return false;
            }

            return true;
        }

        bool InPolygon(int x, int y) const {
            // проводим из точки (x, y) луч, пареллельный оси OX
            auto line1 = line(0, 1, -y);
            int cnt = 0;

            for (const auto& edge : Edges) {
                if (Intersect(x, y, line1, edge.Point1, edge.Point2)) {
                    ++cnt;
                }
            }

            // Если случайный луч из точки (x, y) пересекает 
            // ребра мн-ка нечетное кол-во раз, то точка лежит в мн-ке
            return cnt % 2 != 0; 
        }

        double CountDist(int x, int y, line line) const {
            return (double)abs(line.A * x + line.B * y + line.C) / sqrt(line.A * line.A + line.B * line.B);
        }

        bool IsBorder(int x, int y) const {
            for (const auto& edge : Edges) {
                auto line1 = line(edge.Point1, edge.Point2);
                if (CountDist(x, y, line1) <= (double)BorderWidth) {
                    return true;
                }
            }
            return false;
        }
};