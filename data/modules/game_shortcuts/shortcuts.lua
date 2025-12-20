-- Mobile Shortcuts Module
-- Supports: action buttons with long-press, spell slots, configurable layout

local shortcutsWindow
local shortcuts
local shortcutIcons
local spellBar
local spellSlots

-- Long-press configuration
local LONG_PRESS_TIME = 500 -- ms
local longPressTimers = {}

-- Spell slot configuration
local spellConfig = {}

-- Action definitions for long-press
local longPressActions = {
  attack = function()
    -- Long-press attack: toggle chase mode
    if g_game.isOnline() then
      local player = g_game.getLocalPlayer()
      if player then
        local newChaseMode = player:getChaseMode() == ChaseOpponent and DontChase or ChaseOpponent
        g_game.setChaseMode(newChaseMode)
        triggerHaptic(20)
      end
    end
  end,
  look = function()
    -- Long-press look: inspect (extended look)
    triggerHaptic(20)
  end,
  follow = function()
    -- Long-press follow: stop following
    if g_game.isOnline() then
      g_game.cancelFollow()
      triggerHaptic(20)
    end
  end,
  use = function()
    -- Long-press use: use with crosshair
    triggerHaptic(20)
  end,
  stop = function()
    -- Long-press stop: cancel all actions
    if g_game.isOnline() then
      g_game.cancelAttackAndFollow()
      triggerHaptic(20)
    end
  end,
  chat = function()
    -- Long-press chat: open quick chat wheel
    if modules.game_chatwheel then
      modules.game_chatwheel.show()
    else
      triggerHaptic(20)
    end
  end,
  spell1 = function()
    -- Long-press spell: open spell config
    openSpellConfig(1)
  end,
  spell2 = function()
    openSpellConfig(2)
  end,
  spell3 = function()
    openSpellConfig(3)
  end,
  spell4 = function()
    openSpellConfig(4)
  end
}

function init()
  if not g_platform.isMobile() then return end

  shortcutsWindow = g_ui.displayUI('shortcuts')
  shortcuts = shortcutsWindow.shortcuts
  shortcutIcons = shortcuts.shortcutIcons
  spellBar = shortcutsWindow.spellBar
  spellSlots = spellBar.spellSlots

  connect(g_game, {
    onGameStart = onGameStart,
    onGameEnd = onGameEnd
  })

  loadSpellConfig()
  setupShortcuts()
  setupSpellSlots()
end

function terminate()
  if not g_platform.isMobile() then return end

  -- Clear all timers
  for id, timer in pairs(longPressTimers) do
    removeEvent(timer)
  end
  longPressTimers = {}

  disconnect(g_game, {
    onGameStart = onGameStart,
    onGameEnd = onGameEnd
  })

  saveSpellConfig()
  shortcutsWindow:destroy()
  shortcutsWindow = nil
end

function onGameStart()
  shortcuts:show()
  spellBar:show()
  updateSpellSlots()
end

function onGameEnd()
  shortcuts:hide()
  spellBar:hide()
  resetShortcuts()
end

function hide()
  shortcutsWindow:hide()
end

function show()
  shortcutsWindow:show()
end

-- Spell configuration persistence
function loadSpellConfig()
  local saved = g_settings.getNode('mobileSpells') or {}
  for k, v in pairs(saved) do
    spellConfig[k] = v
  end
end

function saveSpellConfig()
  g_settings.setNode('mobileSpells', spellConfig)
end

function setupShortcuts()
  if not g_platform.isMobile() then return end

  for _, widget in ipairs(shortcutIcons:getChildren()) do
    setupLongPressWidget(widget)
  end
end

function setupSpellSlots()
  if not g_platform.isMobile() then return end

  for _, widget in ipairs(spellSlots:getChildren()) do
    setupLongPressWidget(widget)

    -- Override normal tap for spell slots
    local originalRelease = widget.onMouseRelease
    widget.onMouseRelease = function(self, pos, button)
      if button ~= MouseLeftButton then return false end

      local id = widget:getId()

      -- Cancel long-press timer if still running
      if longPressTimers[id] then
        removeEvent(longPressTimers[id])
        longPressTimers[id] = nil
      end

      -- If it was a long-press, don't do normal tap action
      if widget.isLongPress then
        widget.isLongPress = false
        return true
      end

      -- Cast spell on tap
      castSpell(id)
      triggerHaptic(10)

      return true
    end
  end
end

function setupLongPressWidget(widget)
  widget.image:setChecked(false)
  widget.lastClicked = 0
  widget.isLongPress = false

  -- Touch start - begin long-press timer
  widget.onMousePress = function(self, pos, button)
    if button ~= MouseLeftButton then return false end

    local id = widget:getId()

    -- Cancel any existing timer
    if longPressTimers[id] then
      removeEvent(longPressTimers[id])
    end

    widget.isLongPress = false
    widget.pressStartTime = g_clock.millis()

    -- Start long-press detection timer
    longPressTimers[id] = scheduleEvent(function()
      widget.isLongPress = true
      longPressTimers[id] = nil

      -- Execute long-press action
      if longPressActions[id] then
        longPressActions[id]()

        -- Visual feedback
        widget.image:setBackgroundColor('#FF660066')
        scheduleEvent(function()
          if widget.image:isChecked() then
            widget.image:setBackgroundColor('#00FF0033')
          else
            widget.image:setBackgroundColor('alpha')
          end
        end, 200)
      end
    end, LONG_PRESS_TIME)

    return true
  end

  -- Touch end - handle tap or cancel long-press
  widget.onMouseRelease = function(self, pos, button)
    if button ~= MouseLeftButton then return false end

    local id = widget:getId()

    -- Cancel long-press timer if still running
    if longPressTimers[id] then
      removeEvent(longPressTimers[id])
      longPressTimers[id] = nil
    end

    -- If it was a long-press, don't do normal tap action
    if widget.isLongPress then
      widget.isLongPress = false
      return true
    end

    -- Normal tap behavior - toggle checked state
    if widget.image:isChecked() then
      widget.image:setChecked(false)
    else
      resetShortcuts()
      widget.image:setChecked(true)
      widget.lastClicked = g_clock.millis()
      triggerHaptic(5)
    end

    return true
  end
end

function resetShortcuts()
  for _, widget in ipairs(shortcutIcons:getChildren()) do
    widget.image:setChecked(false)
    widget.lastClicked = 0
    widget.isLongPress = false
  end

  -- Clear all timers
  for id, timer in pairs(longPressTimers) do
    removeEvent(timer)
  end
  longPressTimers = {}
end

function getShortcut()
  for _, widget in ipairs(shortcutIcons:getChildren()) do
    if widget.image:isChecked() then
      return widget:getId()
    end
  end
  return ""
end

function getPanel()
  return shortcuts
end

-- Haptic feedback helper
function triggerHaptic(duration)
  if g_platform.vibrate then
    g_platform.vibrate(duration)
  end
end

-- Spell slot functions
function setSpellSlot(slotId, spellWords, iconPath)
  spellConfig[slotId] = {
    words = spellWords,
    icon = iconPath
  }
  saveSpellConfig()
  updateSpellSlots()
end

function updateSpellSlots()
  if not spellSlots then return end

  for _, widget in ipairs(spellSlots:getChildren()) do
    local id = widget:getId()
    local config = spellConfig[id]

    if config and config.icon then
      widget.image:setImageSource(config.icon)
    else
      widget.image:setImageSource("/images/game/mobile/spell_empty")
    end
  end
end

function castSpell(slotId)
  if not g_game.isOnline() then return end

  local config = spellConfig[slotId]
  if config and config.words then
    g_game.talk(config.words)
  end
end

function openSpellConfig(slotNum)
  triggerHaptic(20)
  -- TODO: Open spell selection UI
  -- For now, just provide haptic feedback
end

-- Execute action by id (can be called from other modules)
function executeAction(actionId)
  if not g_game.isOnline() then return end

  local player = g_game.getLocalPlayer()
  if not player then return end

  if actionId == 'stop' then
    g_game.cancelAttackAndFollow()
  end
end

-- Set custom long-press action for a button
function setLongPressAction(buttonId, callback)
  longPressActions[buttonId] = callback
end

-- Get spell bar panel (for external positioning)
function getSpellBar()
  return spellBar
end
