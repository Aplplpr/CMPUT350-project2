#include "WindowManager.h"

namespace ECSEngine {

WindowManager::WindowManager(unsigned int width, unsigned int height, const std::string& title)
    : mWindow(std::make_shared<sf::RenderWindow>(sf::VideoMode({width, height}), title)), mWorldCenter(width / 2.0f, height / 2.0f), mWorldUnitsPerPixel(1.0f) {}
    //camera initialization aline with world position

std::shared_ptr<sf::RenderWindow> WindowManager::GetWindow() const { return mWindow; }

Rect WindowManager::GetWindowBounds() const {
    sf::Vector2u size = mWindow->getSize();

    Rect bounds;
    bounds.topLeft = Point2D(0.0f, 0.0f);
    bounds.width = (float)size.x;
    bounds.height = (float)size.y;

    return bounds;
}

void WindowManager::DebugDrawRect(const Rect& r, int red, int green, int blue, int alpha) {
    Rect screenRect = WorldToWindow(r);

    sf::RectangleShape shape({screenRect.width, screenRect.height});
    shape.setPosition({screenRect.topLeft.x, screenRect.topLeft.y});
    shape.setFillColor(sf::Color(red, green, blue, alpha));

    mWindow->draw(shape);
}

void WindowManager::DebugFrameRect(const Rect& r, int red, int green, int blue, int alpha) {
    Rect screenRect = WorldToWindow(r);

    sf::RectangleShape shape({screenRect.width, screenRect.height});
    shape.setPosition({screenRect.topLeft.x, screenRect.topLeft.y});

    shape.setFillColor(sf::Color::Transparent);
    shape.setOutlineColor(sf::Color(red, green, blue, alpha));
    shape.setOutlineThickness(1.0f);

    mWindow->draw(shape);
}

Rect WindowManager::WindowToWorld(const Rect& rect) const {
    Rect result;
    result.topLeft = WindowToWorld(rect.topLeft);
    result.width = rect.width * mWorldUnitsPerPixel;
    result.height = rect.height * mWorldUnitsPerPixel;
    return result;
}

Point2D WindowManager::WindowToWorld(const Point2D& pt) const {
    sf::Vector2u size = mWindow->getSize();

    float x = (pt.x - size.x / 2.0f) * mWorldUnitsPerPixel + mWorldCenter.x;
    float y = (pt.y - size.y / 2.0f) * mWorldUnitsPerPixel + mWorldCenter.y;

    return Point2D(x, y);
}

Rect WindowManager::WorldToWindow(const Rect& rect) const {
    Rect result;
    result.topLeft = WorldToWindow(rect.topLeft);
    result.width = rect.width / mWorldUnitsPerPixel;
    result.height = rect.height / mWorldUnitsPerPixel;
    return result;
}

Point2D WindowManager::WorldToWindow(const Point2D& pt) const {
    sf::Vector2u size = mWindow->getSize();

    float x = (pt.x - mWorldCenter.x) / mWorldUnitsPerPixel + size.x / 2.0f;
    float y = (pt.y - mWorldCenter.y) / mWorldUnitsPerPixel + size.y / 2.0f;

    return Point2D(x, y);
}

void WindowManager::SetCamera(const Point2D& worldPt, const Point2D& screenPt) {
    sf::Vector2u size = mWindow->getSize();

    mWorldCenter.x = worldPt.x - (screenPt.x - size.x / 2.0f) * mWorldUnitsPerPixel;
    mWorldCenter.y = worldPt.y - (screenPt.y - size.y / 2.0f) * mWorldUnitsPerPixel;
}

Point2D WindowManager::GetWorldCenter() const {
    return mWorldCenter;
}

float WindowManager::GetWorldScale() const {
    return mWorldUnitsPerPixel;
}

void WindowManager::SetWorldCenter(const Point2D& worldCenter) {
    mWorldCenter = worldCenter;
}

void WindowManager::SetWorldScale(float worldUnitsPerPixel) {
    if (worldUnitsPerPixel > 0) {
        mWorldUnitsPerPixel = worldUnitsPerPixel;
    }
}

void WindowManager::SetWorldRect(const Rect& worldCameraRect) {
    if (worldCameraRect.width <= 0 || worldCameraRect.height <= 0) {
        return;
    }

    sf::Vector2u size = mWindow->getSize();
    if (size.x == 0 || size.y == 0) {
        return;
    }

    float scaleX = worldCameraRect.width / size.x;
    float scaleY = worldCameraRect.height / size.y;

    if (scaleX > scaleY) {
        mWorldUnitsPerPixel = scaleX;
    }
    else {
        mWorldUnitsPerPixel = scaleY;
    }

    mWorldCenter.x = worldCameraRect.topLeft.x + worldCameraRect.width / 2.0f;
    mWorldCenter.y = worldCameraRect.topLeft.y + worldCameraRect.height / 2.0f;
}

float WindowManager::WorldToWindow(float x) const {
    float windowCenterX = mWindow->getSize().x / 2.0f;
    return (x - mWorldCenter.x) / mWorldUnitsPerPixel + windowCenterX;
}

float WindowManager::WindowToWorld(float x) const {
    float windowCenterX = mWindow->getSize().x / 2.0f;
    return (x - windowCenterX) * mWorldUnitsPerPixel + mWorldCenter.x;
}

}  // namespace ECSEngine
