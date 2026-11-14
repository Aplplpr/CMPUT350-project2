
#pragma once

#include <unordered_map>

#include "MathUtil.h"
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>

namespace ECSEngine {

/**
 * @brief Manages the game window and world-to-screen transformations.
 *
 * Provides camera positioning, coordinate conversion, debug drawing,
 * and viewport adjustment to preserve the intended aspect ratio
 * when the window is resized.
 *
 * AI-generated documentation.
 */
class WindowManager {
public:
    /**
     * @brief Creates the game window and initializes the camera.
     *
     * @param width Initial window width in pixels.
     * @param height Initial window height in pixels.
     * @param title Title displayed on the game window.
     *
     * AI-generated documentation.
     */
    WindowManager(unsigned int width, unsigned int height, const std::string& title);

    /**
     * @brief Destroys the window manager.
     *
     * AI-generated documentation.
     */
    ~WindowManager() = default;

    /**
     * @brief Returns the managed SFML window.
     * @return Shared pointer to the render window.
     *
     * AI-generated documentation.
     */
    std::shared_ptr<sf::RenderWindow> GetWindow() const;

    /**
     * @brief Returns the current window dimensions as a rectangle.
     * @return Window bounds in pixel coordinates.
     *
     * AI-generated documentation.
     */
    Rect GetWindowBounds() const;

    /**
     * @brief Aligns a world position with a specified screen position.
     *
     * @param worldPt Position in world coordinates.
     * @param screenPt Target position in window coordinates.
     *
     * AI-generated documentation.
     */
    void SetCamera(const Point2D& worldPt, const Point2D& screenPt);

    /**
     * @brief Sets the center of the camera in world coordinates.
     * @param worldCenter New camera center.
     *
     * AI-generated documentation.
     */
    void SetWorldCenter(const Point2D& worldCenter);

    /**
     * @brief Adjusts the camera to contain a specified world rectangle.
     * @param worldCameraRect Desired visible region in world coordinates.
     *
     * AI-generated documentation.
     */
    void SetWorldRect(const Rect& worldCameraRect);

    /**
     * @brief Sets the number of world units represented by one pixel.
     * @param worldUnitsPerPixel New positive camera scale.
     *
     * AI-generated documentation.
     */
    void SetWorldScale(float worldUnitsPerPixel);

    /**
     * @brief Returns the camera center in world coordinates.
     * @return Current world center.
     *
     * AI-generated documentation.
     */
    Point2D GetWorldCenter() const;

    /**
     * @brief Returns the current camera scale.
     * @return World units per pixel.
     *
     * AI-generated documentation.
     */
    float GetWorldScale() const;

    /**
     * @brief Converts a horizontal window coordinate to world space.
     * @param x Horizontal coordinate in window space.
     * @return Corresponding horizontal world coordinate.
     *
     * AI-generated documentation.
     */
    float WindowToWorld(float x) const;

    /**
     * @brief Converts a horizontal world coordinate to window space.
     * @param x Horizontal coordinate in world space.
     * @return Corresponding horizontal window coordinate.
     *
     * AI-generated documentation.
     */
    float WorldToWindow(float x) const;

    /**
     * @brief Converts a rectangle from window space to world space.
     * @param rect Rectangle in window coordinates.
     * @return Corresponding rectangle in world coordinates.
     *
     * AI-generated documentation.
     */
    Rect WindowToWorld(const Rect& rect) const;

    /**
     * @brief Converts a rectangle from world space to window space.
     * @param rect Rectangle in world coordinates.
     * @return Corresponding rectangle in window coordinates.
     *
     * AI-generated documentation.
     */
    Rect WorldToWindow(const Rect& rect) const;

    /**
     * @brief Converts a point from world space to window space.
     * @param pt Point in world coordinates.
     * @return Corresponding point in window coordinates.
     *
     * AI-generated documentation.
     */
    Point2D WorldToWindow(const Point2D& pt) const;

    /**
     * @brief Converts a point from window space to world space.
     * @param pt Point in window coordinates.
     * @return Corresponding point in world coordinates.
     *
     * AI-generated documentation.
     */
    Point2D WindowToWorld(const Point2D& pt) const;

    /**
     * @brief Draws a filled debugging rectangle.
     *
     * @param r Rectangle in world coordinates.
     * @param red Red color component.
     * @param green Green color component.
     * @param blue Blue color component.
     * @param alpha Transparency component.
     *
     * AI-generated documentation.
     */
    void DebugDrawRect(const Rect& r, int red, int green, int blue, int alpha = 255);

    /**
     * @brief Draws the outline of a debugging rectangle.
     *
     * @param r Rectangle in world coordinates.
     * @param red Red color component.
     * @param green Green color component.
     * @param blue Blue color component.
     * @param alpha Transparency component.
     *
     * AI-generated documentation.
     */
    void DebugFrameRect(const Rect& r, int red, int green, int blue, int alpha = 255);

    /**
     * @brief Updates the viewport to preserve the game's aspect ratio.
     *
     * AI-generated documentation.
     */
    void UpdateViewport();

    /**
     * @brief Returns the original logical game resolution.
     * @return Logical window dimensions.
     *
     * AI-generated documentation.
     */
    sf::Vector2u GetLogicalSize() const;

private:
    std::shared_ptr<sf::RenderWindow> mWindow;
    Point2D mWorldCenter;
    float mWorldUnitsPerPixel;
    sf::Vector2u mLogicalSize;
};

}  // namespace ECSEngine
