
#include "WindowManager.h"

namespace ECSEngine {

/**
 * @brief Creates the rendering window and initializes camera settings.
 *
 * @param width Initial window width in pixels.
 * @param height Initial window height in pixels.
 * @param title Title displayed on the window.
 *
 * Initializes the camera at the center of the window with one
 * world unit per pixel. The original resolution is stored as
 * the logical size and used to configure the initial viewport.
 *
 * AI-generated documentation.
 */
WindowManager::WindowManager(unsigned int width, unsigned int height, const std::string& title)
    : mWindow(std::make_shared<sf::RenderWindow>(sf::VideoMode({width, height}), title)),
      mWorldCenter(width / 2.0f, height / 2.0f),
      mWorldUnitsPerPixel(1.0f),
      mLogicalSize({width, height}) {
    UpdateViewport();
}

    //camera initialization aline with world position

/**
 * @brief Returns the managed SFML render window.
 *
 * @return Shared pointer to the window.
 *
 * AI-generated documentation.
 */
std::shared_ptr<sf::RenderWindow> WindowManager::GetWindow() const { return mWindow; }

/**
 * @brief Retrieves the current window bounds.
 *
 * @return Rectangle starting at the origin with the window's
 * current width and height in pixels.
 *
 * AI-generated documentation.
 */
Rect WindowManager::GetWindowBounds() const {
    sf::Vector2u size = mWindow->getSize();

    Rect bounds;
    bounds.topLeft = Point2D(0.0f, 0.0f);
    bounds.width = (float)size.x;
    bounds.height = (float)size.y;

    return bounds;
}

/**
 * @brief Draws a filled rectangle for debugging.
 *
 * @param r Rectangle in world coordinates.
 * @param red Red color component.
 * @param green Green color component.
 * @param blue Blue color component.
 * @param alpha Transparency component.
 *
 * Converts the rectangle into window coordinates before
 * drawing it with the specified fill color.
 *
 * AI-generated documentation.
 */
void WindowManager::DebugDrawRect(const Rect& r, int red, int green, int blue, int alpha) {
    Rect screenRect = WorldToWindow(r);

    sf::RectangleShape shape({screenRect.width, screenRect.height});
    shape.setPosition({screenRect.topLeft.x, screenRect.topLeft.y});
    shape.setFillColor(sf::Color(red, green, blue, alpha));

    mWindow->draw(shape);
}

/**
 * @brief Draws a rectangle outline for debugging.
 *
 * @param r Rectangle in world coordinates.
 * @param red Red color component.
 * @param green Green color component.
 * @param blue Blue color component.
 * @param alpha Transparency component.
 *
 * Converts the rectangle into window coordinates and draws
 * a transparent shape with a one-pixel outline.
 *
 * AI-generated documentation.
 */
void WindowManager::DebugFrameRect(const Rect& r, int red, int green, int blue, int alpha) {
    Rect screenRect = WorldToWindow(r);

    sf::RectangleShape shape({screenRect.width, screenRect.height});
    shape.setPosition({screenRect.topLeft.x, screenRect.topLeft.y});

    shape.setFillColor(sf::Color::Transparent);
    shape.setOutlineColor(sf::Color(red, green, blue, alpha));
    shape.setOutlineThickness(1.0f);

    mWindow->draw(shape);
}

/**
 * @brief Converts a rectangle from window space to world space.
 *
 * @param rect Rectangle in window coordinates.
 * @return Converted rectangle in world coordinates.
 *
 * Converts the top-left position and scales the dimensions
 * according to the current world-units-per-pixel value.
 *
 * AI-generated documentation.
 */
Rect WindowManager::WindowToWorld(const Rect& rect) const {
    Rect result;
    result.topLeft = WindowToWorld(rect.topLeft);
    result.width = rect.width * mWorldUnitsPerPixel;
    result.height = rect.height * mWorldUnitsPerPixel;
    return result;
}

/**
 * @brief Converts a point from window space to world space.
 *
 * @param pt Point in window coordinates.
 * @return Corresponding point in world coordinates.
 *
 * Uses the window center, camera center, and camera scale
 * to determine the point's position in the game world.
 *
 * AI-generated documentation.
 */
Point2D WindowManager::WindowToWorld(const Point2D& pt) const {
    sf::Vector2u size = mWindow->getSize();

    float x = (pt.x - size.x / 2.0f) * mWorldUnitsPerPixel + mWorldCenter.x;
    float y = (pt.y - size.y / 2.0f) * mWorldUnitsPerPixel + mWorldCenter.y;

    return Point2D(x, y);
}

/**
 * @brief Converts a rectangle from world space to window space.
 *
 * @param rect Rectangle in world coordinates.
 * @return Converted rectangle in window coordinates.
 *
 * Converts the top-left position and scales the dimensions
 * from world units into pixels.
 *
 * AI-generated documentation.
 */
Rect WindowManager::WorldToWindow(const Rect& rect) const {
    Rect result;
    result.topLeft = WorldToWindow(rect.topLeft);
    result.width = rect.width / mWorldUnitsPerPixel;
    result.height = rect.height / mWorldUnitsPerPixel;
    return result;
}

/**
 * @brief Converts a point from world space to window space.
 *
 * @param pt Point in world coordinates.
 * @return Corresponding point in window coordinates.
 *
 * Calculates the point's position relative to the camera
 * center and applies the current camera scale.
 *
 * AI-generated documentation.
 */
Point2D WindowManager::WorldToWindow(const Point2D& pt) const {
    sf::Vector2u size = mWindow->getSize();

    float x = (pt.x - mWorldCenter.x) / mWorldUnitsPerPixel + size.x / 2.0f;
    float y = (pt.y - mWorldCenter.y) / mWorldUnitsPerPixel + size.y / 2.0f;

    return Point2D(x, y);
}

/**
 * @brief Positions the camera to align a world point with a screen point.
 *
 * @param worldPt Target point in world coordinates.
 * @param screenPt Desired position in window coordinates.
 *
 * Updates the camera center so that the chosen world point
 * appears at the specified screen position.
 *
 * AI-generated documentation.
 */
void WindowManager::SetCamera(const Point2D& worldPt, const Point2D& screenPt) {
    sf::Vector2u size = mWindow->getSize();

    mWorldCenter.x = worldPt.x - (screenPt.x - size.x / 2.0f) * mWorldUnitsPerPixel;
    mWorldCenter.y = worldPt.y - (screenPt.y - size.y / 2.0f) * mWorldUnitsPerPixel;
}

/**
 * @brief Returns the current camera center.
 *
 * @return Camera center in world coordinates.
 *
 * AI-generated documentation.
 */
Point2D WindowManager::GetWorldCenter() const {
    return mWorldCenter;
}

/**
 * @brief Returns the current camera scale.
 *
 * @return Number of world units represented by one pixel.
 *
 * AI-generated documentation.
 */
float WindowManager::GetWorldScale() const {
    return mWorldUnitsPerPixel;
}

/**
 * @brief Sets the camera center in world coordinates.
 *
 * @param worldCenter New camera center.
 *
 * AI-generated documentation.
 */
void WindowManager::SetWorldCenter(const Point2D& worldCenter) {
    mWorldCenter = worldCenter;
}

/**
 * @brief Sets the camera scale when the given value is positive.
 *
 * @param worldUnitsPerPixel Number of world units represented by one pixel.
 *
 * AI-generated documentation.
 */
void WindowManager::SetWorldScale(float worldUnitsPerPixel) {
    if (worldUnitsPerPixel > 0) {
        mWorldUnitsPerPixel = worldUnitsPerPixel;
    }
}

/**
 * @brief Adjusts the camera to contain a world-space rectangle.
 *
 * @param worldCameraRect Desired visible region in world coordinates.
 *
 * Calculates horizontal and vertical scale factors and chooses
 * the larger value so the entire rectangle fits within the
 * window. The camera center is then moved to the rectangle's center.
 * Invalid rectangle dimensions or zero-sized windows are ignored.
 *
 * AI-generated documentation.
 */
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

/**
 * @brief Converts a horizontal world coordinate to window space.
 *
 * @param x Horizontal coordinate in world space.
 * @return Corresponding horizontal window coordinate.
 *
 * AI-generated documentation.
 */
float WindowManager::WorldToWindow(float x) const {
    float windowCenterX = mWindow->getSize().x / 2.0f;
    return (x - mWorldCenter.x) / mWorldUnitsPerPixel + windowCenterX;
}

/**
 * @brief Converts a horizontal window coordinate to world space.
 *
 * @param x Horizontal coordinate in window space.
 * @return Corresponding horizontal world coordinate.
 *
 * AI-generated documentation.
 */
float WindowManager::WindowToWorld(float x) const {
    float windowCenterX = mWindow->getSize().x / 2.0f;
    return (x - windowCenterX) * mWorldUnitsPerPixel + mWorldCenter.x;
}

/**
 * @brief Updates the viewport to preserve the logical aspect ratio.
 *
 * Compares the current window aspect ratio with the original
 * game resolution. If the window is wider than the game,
 * the viewport width is reduced to add horizontal margins.
 * Otherwise, its height is reduced to add vertical margins.
 *
 * The viewport is centered within the window, preventing
 * the game image from stretching when the window is resized.
 *
 * AI-generated documentation.
 */
void WindowManager::UpdateViewport() {
    sf::Vector2u size = mWindow->getSize();
    if (size.x == 0 || size.y == 0) return;

    float windowRatio = (float)size.x / size.y;
    float gameRatio = (float)mLogicalSize.x / mLogicalSize.y;

    float viewportWidth = 1.0f;
    float viewportHeight = 1.0f;

    if (windowRatio > gameRatio) {
        viewportWidth = gameRatio / windowRatio;
    }
    else {
        viewportHeight = windowRatio / gameRatio;
    }

    sf::View view(sf::FloatRect(
        {0.0f, 0.0f},
        {(float)mLogicalSize.x, (float)mLogicalSize.y}
    ));

    view.setViewport(sf::FloatRect(
        {(1.0f - viewportWidth) / 2.0f,
         (1.0f - viewportHeight) / 2.0f},
        {viewportWidth, viewportHeight}
    ));

    mWindow->setView(view);
}

/**
 * @brief Returns the original logical game resolution.
 *
 * @return Logical width and height used for viewport calculations.
 *
 * AI-generated documentation.
 */
sf::Vector2u WindowManager::GetLogicalSize() const {
    return mLogicalSize;
}

}  // namespace ECSEngine
