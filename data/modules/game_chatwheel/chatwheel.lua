-- Mobile Quick Chat Wheel Module
-- Provides a radial menu for quick chat messages

local chatWheelWindow
local wheel
local backdrop
local centerLabel

-- Default chat options (can be customized)
local chatOptions = {
  option1 = { text = "Hi!", message = "Hi!" },
  option2 = { text = "Thanks!", message = "Thank you!" },
  option3 = { text = "Help!", message = "Help please!" },
  option4 = { text = "Sorry", message = "Sorry!" },
  option5 = { text = "Bye!", message = "Bye!" },
  option6 = { text = "Wait", message = "Wait please" },
  option7 = { text = "Yes", message = "Yes" },
  option8 = { text = "No", message = "No" }
}

local isVisible = false

function init()
  if not g_platform.isMobile() then return end

  chatWheelWindow = g_ui.displayUI('chatwheel')
  wheel = chatWheelWindow.wheel
  backdrop = chatWheelWindow.backdrop
  centerLabel = wheel.centerLabel

  -- Load saved chat options
  loadChatOptions()

  -- Setup option buttons
  setupOptions()

  -- Setup backdrop to close wheel
  backdrop.onClick = function()
    hide()
  end

  connect(g_game, {
    onGameEnd = onGameEnd
  })
end

function terminate()
  if not g_platform.isMobile() then return end

  saveChatOptions()

  disconnect(g_game, {
    onGameEnd = onGameEnd
  })

  chatWheelWindow:destroy()
  chatWheelWindow = nil
end

function onGameEnd()
  hide()
end

function loadChatOptions()
  local saved = g_settings.getNode('chatWheel') or {}
  for k, v in pairs(saved) do
    if chatOptions[k] then
      chatOptions[k] = v
    end
  end

  -- Update button texts
  if wheel then
    for id, config in pairs(chatOptions) do
      local btn = wheel:getChildById(id)
      if btn then
        btn:setText(config.text)
      end
    end
  end
end

function saveChatOptions()
  g_settings.setNode('chatWheel', chatOptions)
end

function setupOptions()
  for id, config in pairs(chatOptions) do
    local btn = wheel:getChildById(id)
    if btn then
      btn:setText(config.text)

      btn.onClick = function()
        sendMessage(config.message)
        triggerHaptic(10)
        hide()
      end

      btn.onMousePress = function(self, pos, button)
        if button == MouseLeftButton then
          centerLabel:setText(config.text)
          triggerHaptic(5)
        end
        return true
      end
    end
  end
end

function sendMessage(message)
  if not g_game.isOnline() then return end

  -- Send as normal say
  g_game.talk(message)
end

function show()
  if not chatWheelWindow then return end

  chatWheelWindow:show()
  chatWheelWindow:raise()
  isVisible = true
  triggerHaptic(15)
end

function hide()
  if not chatWheelWindow then return end

  chatWheelWindow:hide()
  isVisible = false
  centerLabel:setText("Quick Chat")
end

function toggle()
  if isVisible then
    hide()
  else
    show()
  end
end

function isShown()
  return isVisible
end

-- Set custom chat option
function setChatOption(optionId, text, message)
  if chatOptions[optionId] then
    chatOptions[optionId] = { text = text, message = message }

    local btn = wheel:getChildById(optionId)
    if btn then
      btn:setText(text)
    end

    saveChatOptions()
  end
end

-- Get chat option
function getChatOption(optionId)
  return chatOptions[optionId]
end

-- Haptic feedback helper
function triggerHaptic(duration)
  if g_platform.vibrate then
    g_platform.vibrate(duration)
  end
end
