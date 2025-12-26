-- Floating Joystick Module for Mobile
-- Supports: floating position, haptic feedback, direction indicators, settings

local overlay
local touchZone
local keypad
local pointer
local keypadUpdateEvent
local snapBackEvent
local autoHideEvent
local moveListener

-- Joystick state
local isActive = false
local spawnPos = {x = 0, y = 0}
local currentPos = {x = 0, y = 0}
local currentDir = nil
local lastDir = nil

-- Settings (can be modified via options)
local settings = {
  opacity = 0.8,
  size = 150,
  deadZone = 0.15,      -- 15% dead zone in center
  sensitivity = 1.0,
  leftHanded = false,   -- false = joystick on left
  autoHide = true,
  autoHideDelay = 3000, -- ms before auto-hide
  hapticEnabled = true,
  floatingMode = false   -- true = floating, false = fixed position
}

-- Direction colors for visual feedback
local dirHighlight = "#FFFFFF44"
local dirNormal = "alpha"

function init()
  g_logger.info("game_joystick: init() called, isMobile=" .. tostring(g_platform.isMobile()))
  if not g_platform.isMobile() then return end

  overlay = g_ui.displayUI('joystick')
  if not overlay then
    g_logger.error("game_joystick: Failed to load joystick UI")
    return
  end
  g_logger.info("game_joystick: overlay created successfully")

  touchZone = overlay:getChildById('touchZone')
  keypad = overlay:getChildById('keypad')

  if not touchZone or not keypad then
    g_logger.error("game_joystick: Failed to find touchZone or keypad in joystick UI")
    return
  end
  g_logger.info("game_joystick: touchZone and keypad found")

  pointer = keypad:getChildById('pointer')

  -- Load saved settings
  loadSettings()
  -- Force fixed mode on mobile for now (override any saved floating mode)
  settings.floatingMode = false
  applySettings()
  g_logger.info("game_joystick: settings applied, floatingMode=" .. tostring(settings.floatingMode))

  connect(touchZone, {
    onMousePress = onTouchPress,
    onMouseRelease = onTouchRelease,
    onMouseMove = onTouchMove
  })

  -- Also connect events to keypad for fixed mode (keypad receives direct touches)
  connect(keypad, {
    onMousePress = onTouchPress,
    onMouseRelease = onTouchRelease,
    onMouseMove = onTouchMove
  })

  connect(g_game, {
    onGameStart = onGameStart,
    onGameEnd = onGameEnd
  })
  g_logger.info("game_joystick: init() completed, game online=" .. tostring(g_game.isOnline()))

  -- If game is already online, show joystick now
  if g_game.isOnline() then
    g_logger.info("game_joystick: Game already online, calling onGameStart")
    onGameStart()
  end
end

function terminate()
  if not g_platform.isMobile() then return end

  removeEvent(keypadUpdateEvent)
  removeEvent(snapBackEvent)
  removeEvent(autoHideEvent)
  removeEvent(showKeypadFixedRetryEvent)

  if touchZone then
    disconnect(touchZone, {
      onMousePress = onTouchPress,
      onMouseRelease = onTouchRelease,
      onMouseMove = onTouchMove
    })
  end

  if keypad then
    disconnect(keypad, {
      onMousePress = onTouchPress,
      onMouseRelease = onTouchRelease,
      onMouseMove = onTouchMove
    })
  end

  disconnect(g_game, {
    onGameStart = onGameStart,
    onGameEnd = onGameEnd
  })

  saveSettings()
  if overlay then
    overlay:destroy()
    overlay = nil
  end
  touchZone = nil
  keypad = nil
  pointer = nil
end

function hide()
  if overlay then overlay:hide() end
end

function show()
  if overlay then overlay:show() end
end

function onGameStart()
  -- Force fixed mode - something keeps resetting this
  settings.floatingMode = false
  g_logger.info("game_joystick: onGameStart() called, floatingMode=" .. tostring(settings.floatingMode))

  -- Raise overlay to front so it receives touch events above game panels
  if overlay then
    overlay:raise()
    g_logger.info("game_joystick: overlay:raise() called")
  end

  if settings.floatingMode then
    -- Floating mode: show touchZone to detect where user touches
    if touchZone then
      touchZone:show()
      touchZone:raise()
      g_logger.info("game_joystick: touchZone:show() for floating mode")
    end
  else
    -- Fixed mode: hide touchZone (it would block the console), use keypad directly
    if touchZone then
      touchZone:hide()
      g_logger.info("game_joystick: touchZone:hide() for fixed mode")
    end
    g_logger.info("game_joystick: calling showKeypadFixed()")
    showKeypadFixed()
  end
end

function onGameEnd()
  if touchZone then touchZone:hide() end
  if keypad then keypad:hide() end
  isActive = false
end

-- Settings management
function loadSettings()
  local saved = g_settings.getNode('joystick') or {}
  for k, v in pairs(saved) do
    -- Don't load floatingMode from saved settings - always use fixed mode on mobile
    if settings[k] ~= nil and k ~= 'floatingMode' then
      settings[k] = v
    end
  end
end

function saveSettings()
  g_settings.setNode('joystick', settings)
end

function applySettings()
  -- Guard: ensure UI elements exist
  if not keypad or not touchZone then
    return
  end

  -- Apply opacity
  keypad:setOpacity(settings.opacity)

  -- Apply size
  keypad:setSize({width = settings.size, height = settings.size})

  -- Apply left-handed mode
  if settings.leftHanded then
    touchZone:breakAnchors()
    touchZone:addAnchor(AnchorTop, 'parent', AnchorTop)
    touchZone:addAnchor(AnchorBottom, 'parent', AnchorBottom)
    touchZone:addAnchor(AnchorRight, 'parent', AnchorRight)
  else
    touchZone:breakAnchors()
    touchZone:addAnchor(AnchorTop, 'parent', AnchorTop)
    touchZone:addAnchor(AnchorBottom, 'parent', AnchorBottom)
    touchZone:addAnchor(AnchorLeft, 'parent', AnchorLeft)
  end
end

function getSetting(key)
  return settings[key]
end

function setSetting(key, value)
  if settings[key] ~= nil then
    settings[key] = value
    applySettings()
    saveSettings()
  end
end

-- Show keypad at fixed position (non-floating mode)
local showKeypadFixedRetryEvent = nil

function showKeypadFixed()
  g_logger.info("game_joystick: showKeypadFixed() called")
  if not overlay or not keypad then
    g_logger.error("game_joystick: overlay or keypad is nil in showKeypadFixed")
    return
  end
  local parent = overlay
  local marginHorizontal = 20
  local marginBottom = 80  -- Higher margin to avoid overlapping with console at bottom

  local parentWidth = parent:getWidth()
  local parentHeight = parent:getHeight()
  g_logger.info("game_joystick: parent size=" .. parentWidth .. "x" .. parentHeight)

  -- If parent dimensions are not ready yet, retry after a short delay
  if parentWidth == 0 or parentHeight == 0 then
    g_logger.info("game_joystick: parent dimensions not ready, scheduling retry...")
    removeEvent(showKeypadFixedRetryEvent)
    showKeypadFixedRetryEvent = scheduleEvent(showKeypadFixed, 100)
    return
  end

  local x, y
  if settings.leftHanded then
    x = parentWidth - settings.size - marginHorizontal
    y = parentHeight - settings.size - marginBottom
  else
    x = marginHorizontal
    y = parentHeight - settings.size - marginBottom
  end

  g_logger.info("game_joystick: setting keypad position to " .. x .. "," .. y .. " size=" .. settings.size)
  keypad:setPosition({x = x, y = y})
  keypad:show()
  keypad:raise()  -- Ensure keypad is on top
  g_logger.info("game_joystick: keypad:show() called, keypad visible=" .. tostring(keypad:isVisible()))
  resetAutoHide()
end

-- Show keypad at touch position (floating mode)
function showKeypadAtPosition(pos)
  if not overlay or not keypad then return end

  -- Center the keypad on touch position
  local halfSize = settings.size / 2
  local x = pos.x - halfSize
  local y = pos.y - halfSize

  -- Clamp to screen bounds
  local parent = overlay
  x = math.max(0, math.min(x, parent:getWidth() - settings.size))
  y = math.max(0, math.min(y, parent:getHeight() - settings.size))

  keypad:setPosition({x = x, y = y})
  keypad:setOpacity(settings.opacity)
  keypad:show()

  spawnPos = {x = pos.x, y = pos.y}
end

-- Touch handlers
function onTouchPress(widget, pos, button)
  if button ~= MouseLeftButton then return false end

  isActive = true
  currentPos = {x = pos.x, y = pos.y}

  removeEvent(autoHideEvent)
  removeEvent(snapBackEvent)

  if settings.floatingMode then
    showKeypadAtPosition(pos)
  end

  -- Trigger haptic feedback
  if settings.hapticEnabled then
    triggerHaptic(10) -- light tap
  end

  executeWalk(true)
  return true
end

function onTouchMove(widget, pos, offset)
  if not isActive then return false end

  currentPos = {x = pos.x, y = pos.y}
  return true
end

function onTouchRelease(widget, pos, button)
  if button ~= MouseLeftButton then return false end

  isActive = false
  currentPos = {x = pos.x, y = pos.y}

  removeEvent(keypadUpdateEvent)
  keypadUpdateEvent = nil

  -- Animate snap-back
  animateSnapBack()

  -- Clear direction indicators
  clearDirectionIndicators()

  -- Start auto-hide timer
  resetAutoHide()

  return true
end

-- Snap-back animation
function animateSnapBack()
  if not pointer then return end

  local steps = 5
  local currentStep = 0
  local startMarginTop = pointer:getMarginTop()
  local startMarginLeft = pointer:getMarginLeft()

  local function doStep()
    if not pointer then return end

    currentStep = currentStep + 1
    local progress = currentStep / steps
    local easeProgress = 1 - math.pow(1 - progress, 3) -- ease out cubic

    pointer:setMarginTop(startMarginTop * (1 - easeProgress))
    pointer:setMarginLeft(startMarginLeft * (1 - easeProgress))

    if currentStep < steps then
      snapBackEvent = scheduleEvent(doStep, 16) -- ~60fps
    else
      pointer:setMarginTop(0)
      pointer:setMarginLeft(0)

      if settings.floatingMode then
        -- Fade out in floating mode
        animateFadeOut()
      end
    end
  end

  doStep()
end

function animateFadeOut()
  if not settings.floatingMode or isActive or not keypad then return end

  local steps = 10
  local currentStep = 0
  local startOpacity = keypad:getOpacity()

  local function doStep()
    if isActive or not keypad then return end -- cancelled by new touch or destroyed

    currentStep = currentStep + 1
    local progress = currentStep / steps

    keypad:setOpacity(startOpacity * (1 - progress))

    if currentStep < steps then
      scheduleEvent(doStep, 30)
    else
      keypad:hide()
      keypad:setOpacity(settings.opacity)
    end
  end

  scheduleEvent(doStep, 200) -- delay before fade
end

-- Auto-hide functionality
function resetAutoHide()
  removeEvent(autoHideEvent)

  if settings.autoHide and not settings.floatingMode then
    autoHideEvent = scheduleEvent(function()
      if not isActive then
        animateFadeOut()
      end
    end, settings.autoHideDelay)
  end
end

-- Direction indicator updates
function updateDirectionIndicators(dir)
  clearDirectionIndicators()

  if dir == Directions.North or dir == Directions.NorthWest or dir == Directions.NorthEast then
    keypad.dirNorth:setBackgroundColor(dirHighlight)
  end
  if dir == Directions.South or dir == Directions.SouthWest or dir == Directions.SouthEast then
    keypad.dirSouth:setBackgroundColor(dirHighlight)
  end
  if dir == Directions.West or dir == Directions.NorthWest or dir == Directions.SouthWest then
    keypad.dirWest:setBackgroundColor(dirHighlight)
  end
  if dir == Directions.East or dir == Directions.NorthEast or dir == Directions.SouthEast then
    keypad.dirEast:setBackgroundColor(dirHighlight)
  end
end

function clearDirectionIndicators()
  if keypad.dirNorth then keypad.dirNorth:setBackgroundColor(dirNormal) end
  if keypad.dirSouth then keypad.dirSouth:setBackgroundColor(dirNormal) end
  if keypad.dirWest then keypad.dirWest:setBackgroundColor(dirNormal) end
  if keypad.dirEast then keypad.dirEast:setBackgroundColor(dirNormal) end
end

-- Haptic feedback (requires native support)
function triggerHaptic(duration)
  if not settings.hapticEnabled then return end

  -- Call native haptic if available
  if g_platform.vibrate then
    g_platform.vibrate(duration)
  end
end

-- Main walk execution logic
function executeWalk(isFirstStep)
  removeEvent(keypadUpdateEvent)
  keypadUpdateEvent = nil

  if not isActive or not keypad then
    return
  end

  keypadUpdateEvent = scheduleEvent(function() executeWalk(false) end, 20)

  -- Calculate position relative to keypad center
  local keypadPos = keypad:getPosition()
  local keypadCenter = {
    x = keypadPos.x + settings.size / 2,
    y = keypadPos.y + settings.size / 2
  }

  -- Normalized position (-1 to 1)
  local normalX = (currentPos.x - keypadCenter.x) / (settings.size / 2) * settings.sensitivity
  local normalY = (currentPos.y - keypadCenter.y) / (settings.size / 2) * settings.sensitivity

  -- Clamp to -1 to 1
  normalX = math.max(-1, math.min(1, normalX))
  normalY = math.max(-1, math.min(1, normalY))

  -- Update pointer position (visual)
  if pointer then
    local maxOffset = settings.size / 2 - 25 -- 25 = half pointer size
    pointer:setMarginLeft(normalX * maxOffset)
    pointer:setMarginTop(normalY * maxOffset)
  end

  -- Check dead zone
  local distance = math.sqrt(normalX * normalX + normalY * normalY)
  if distance < settings.deadZone then
    clearDirectionIndicators()
    currentDir = nil
    return
  end

  -- Determine direction
  local dir = nil
  local angle = math.atan2(normalX, normalY)
  local deg = math.deg(angle)

  -- 8-directional with 45 degree sectors
  if deg >= -22.5 and deg < 22.5 then
    dir = Directions.South
  elseif deg >= 22.5 and deg < 67.5 then
    dir = Directions.SouthEast
  elseif deg >= 67.5 and deg < 112.5 then
    dir = Directions.East
  elseif deg >= 112.5 and deg < 157.5 then
    dir = Directions.NorthEast
  elseif deg >= 157.5 or deg < -157.5 then
    dir = Directions.North
  elseif deg >= -157.5 and deg < -112.5 then
    dir = Directions.NorthWest
  elseif deg >= -112.5 and deg < -67.5 then
    dir = Directions.West
  elseif deg >= -67.5 and deg < -22.5 then
    dir = Directions.SouthWest
  end

  -- Update visual indicators
  updateDirectionIndicators(dir)

  -- Haptic feedback on direction change
  if dir ~= lastDir and dir ~= nil then
    triggerHaptic(5)
    lastDir = dir
  end

  currentDir = dir

  -- Notify listener
  if dir and moveListener then
    moveListener(dir, isFirstStep)
  end
end

-- Public API
function addOnJoystickMoveListener(callback)
  moveListener = callback
end

function getPanel()
  return keypad
end

function isFloatingMode()
  return settings.floatingMode
end

function setFloatingMode(enabled)
  settings.floatingMode = enabled
  saveSettings()

  if not enabled and g_game.isOnline() then
    showKeypadFixed()
  end
end

function getSettings()
  return settings
end
