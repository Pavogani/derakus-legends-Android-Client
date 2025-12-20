-- Auto-hide UI module for mobile-friendly gameplay
-- Hides UI panels after 5 seconds of inactivity
-- Health/Mana bars remain visible

-- Configuration
local AUTO_HIDE_DELAY = 5000  -- 5 seconds before hiding
local FADE_DURATION = 300     -- ms fade animation

-- State
local hideTimer = nil
local panelsHidden = false
local enabled = false

-- Panels to hide (excludes health/mana bars)
local hiddenPanels = {}

-- Get panels that should be hidden
local function getHideablePanels()
    local panels = {}
    local gameInterface = modules.game_interface

    if gameInterface then
        -- Add side panels
        local rightPanel = gameInterface.getRightPanel()
        local leftPanel = gameInterface.getLeftPanel()
        local rightExtraPanel = gameInterface.getRightExtraPanel()
        local leftExtraPanel = gameInterface.getLeftExtraPanel()
        local bottomPanel = gameInterface.getBottomPanel()

        if rightPanel then table.insert(panels, rightPanel) end
        if leftPanel then table.insert(panels, leftPanel) end
        if rightExtraPanel then table.insert(panels, rightExtraPanel) end
        if leftExtraPanel then table.insert(panels, leftExtraPanel) end
        if bottomPanel then table.insert(panels, bottomPanel) end
    end

    return panels
end

-- Hide UI panels with fade effect
local function hideUI()
    if panelsHidden or not enabled then
        return
    end

    hiddenPanels = getHideablePanels()

    for _, panel in ipairs(hiddenPanels) do
        if panel and panel:isVisible() then
            g_effects.fadeOut(panel, FADE_DURATION)
        end
    end

    panelsHidden = true
end

-- Show UI panels with fade effect
local function showUI()
    if not panelsHidden then
        return
    end

    for _, panel in ipairs(hiddenPanels) do
        if panel then
            g_effects.cancelFade(panel)
            panel:setOpacity(1.0)
        end
    end

    panelsHidden = false
end

-- Reset the hide timer (called on user interaction)
local function resetHideTimer()
    -- Show UI immediately if hidden
    if panelsHidden then
        showUI()
    end

    -- Cancel existing timer
    if hideTimer then
        removeEvent(hideTimer)
        hideTimer = nil
    end

    -- Start new timer if enabled
    if enabled then
        hideTimer = scheduleEvent(hideUI, AUTO_HIDE_DELAY)
    end
end

-- Input event handlers
local function onKeyDown(keyCode, keyboardModifiers)
    resetHideTimer()
    return false -- Don't consume the event
end

local function onKeyPress(keyCode, keyboardModifiers, autoRepeatTicks)
    resetHideTimer()
    return false
end

local function onMousePress(mousePos, button)
    resetHideTimer()
    return false
end

local function onMouseMove(mousePos, mouseMoved)
    resetHideTimer()
    return false
end

-- Enable/disable auto-hide
function setAutoHideEnabled(value)
    enabled = value
    g_settings.set('autoHideUI', enabled)

    if enabled then
        resetHideTimer()
    else
        -- Show UI and cancel timer when disabled
        if hideTimer then
            removeEvent(hideTimer)
            hideTimer = nil
        end
        if panelsHidden then
            showUI()
        end
    end
end

function isAutoHideEnabled()
    return enabled
end

function toggle()
    setAutoHideEnabled(not enabled)
end

-- Module initialization
function init()
    -- Load setting (default: disabled, but enabled on mobile)
    local defaultEnabled = g_platform.isMobile()
    enabled = g_settings.getBoolean('autoHideUI', defaultEnabled)

    -- Connect to game events
    connect(g_game, {
        onGameStart = onGameStart,
        onGameEnd = onGameEnd
    })

    -- If game is already running, start the module
    if g_game.isOnline() then
        onGameStart()
    end
end

function terminate()
    -- Clean up timer
    if hideTimer then
        removeEvent(hideTimer)
        hideTimer = nil
    end

    -- Show UI before terminating
    if panelsHidden then
        for _, panel in ipairs(hiddenPanels) do
            if panel then
                g_effects.cancelFade(panel)
                panel:setOpacity(1.0)
            end
        end
        panelsHidden = false
    end

    -- Disconnect from game events
    disconnect(g_game, {
        onGameStart = onGameStart,
        onGameEnd = onGameEnd
    })

    -- Disconnect input handlers
    onGameEnd()
end

function onGameStart()
    if not enabled then
        return
    end

    -- Connect input handlers to root widget
    local gameInterface = modules.game_interface
    if gameInterface then
        local rootPanel = gameInterface.getRootPanel()
        if rootPanel then
            connect(rootPanel, {
                onKeyDown = onKeyDown,
                onKeyPress = onKeyPress,
                onMousePress = onMousePress,
                onMouseMove = onMouseMove
            })
        end
    end

    -- Start the hide timer
    resetHideTimer()
end

function onGameEnd()
    -- Cancel timer
    if hideTimer then
        removeEvent(hideTimer)
        hideTimer = nil
    end

    -- Show UI
    if panelsHidden then
        for _, panel in ipairs(hiddenPanels) do
            if panel then
                g_effects.cancelFade(panel)
                panel:setOpacity(1.0)
            end
        end
        panelsHidden = false
    end

    -- Disconnect input handlers
    local gameInterface = modules.game_interface
    if gameInterface then
        local rootPanel = gameInterface.getRootPanel()
        if rootPanel then
            disconnect(rootPanel, {
                onKeyDown = onKeyDown,
                onKeyPress = onKeyPress,
                onMousePress = onMousePress,
                onMouseMove = onMouseMove
            })
        end
    end
end
