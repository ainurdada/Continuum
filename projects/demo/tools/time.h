#pragma once

#include <assert.h>
#include <cmath>

struct Time {
  private:
    double _realDeltaSeconds{};
    double _realElapsedSeconds{};
    double _simulationDeltaSeconds{};
    double _simulationElapsedSeconds{};
    double _timeScale = 1;
    bool _paused{};

  public:
    void addTime(double realDeltaSeconds) {
        assert(realDeltaSeconds >= 0);
        assert(std::isfinite(realDeltaSeconds));

        _realDeltaSeconds = realDeltaSeconds;
        _realElapsedSeconds += _realDeltaSeconds;
        if (!_paused) {
            _simulationDeltaSeconds = _realDeltaSeconds * _timeScale;
            _simulationElapsedSeconds += _simulationDeltaSeconds;
        } else {
            _simulationDeltaSeconds = 0;
        }
    }

    inline double realDeltaSeconds() const noexcept {
        return _realDeltaSeconds;
    }
    inline double realElapsedSeconds() const noexcept {
        return _realElapsedSeconds;
    }
    inline double simulationDeltaSeconds() const noexcept {
        return _simulationDeltaSeconds;
    }
    inline double simulationElapsedSeconds() const noexcept {
        return _simulationElapsedSeconds;
    }
    inline double timeScale() const noexcept {
        return _timeScale;
    }
    bool timeScale(double newScale) {
        if (newScale > 0 && std::isfinite(newScale)) {
            _timeScale = newScale;
            return true;
        }
        return false;
    }
    inline bool pause() const noexcept {
        return _paused;
    }
    void pause(bool newPaused) {
        _paused = newPaused;
    }
};