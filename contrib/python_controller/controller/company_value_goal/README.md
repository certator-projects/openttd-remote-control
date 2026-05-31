# Company value goal game mode controller

This is a full description of how controller should work.

## Game state

Controller script should mirror state of the game in local sqlite database (use SQLAlchemy layer). Controller should be able to distinguish different game sessions, and store state separately per each session. For simplicity, there will be a separate sqlite file per each session, containing UTC ISO timestamp in its name. Unless controller scripts detects that this is a new session, it expects that sqlite file with latest UTC ISO timestamp in name is associated with current session.

## OpenTTD gRPC connection

Controller script is expecting that there will be a dedicated openTTD server running with gRPC API available. In the case it is not reachable, controller script will periodically ping it. Once it is reachable, it is gets back to normal operation.

## Game session

Unless controller determines that game mode is GameModeType.MULTIPLAYER_SERVER, it assumes that openTTD is in standby mode, and there is no live game session. In such case, it should create a new session and start network game.

## Life cycle

### Initial start

Initially when server starts, it is expected that openTTD will create default company without connected clients. Controller script should remove that company with reset_company RPC method.

Controller also should create a global game goal "Reach 10M company value" via `ScriptGoal.New`. The response includes the real `goal_id` once the game script finishes creating the goal (the gRPC server handles deferred completion internally).

### Network client (player)

When network client connects, controller script should greet them with a public chat message. Then, with a little delay, it should send game instructions via `ScriptGoal.QuestionClient` (in-game popup with a Start button), falling back to private chat if the question RPC fails.

### Player's company

By design, there is no strict coupling between player and a company:
- if player is spectator and not associated with any company, it can create a new company
- any player can join any company
- company can have multiple players joined at the same time
- company may have none players joined, after they disconnect

### Idling

Controller script should periodically (every second) sync openTTD game state with sqlite storage.

Controller script should periodically (every 5 minutes) calculate how close each company to the 10M value, and output sorted list to the public chat.

### Game end

Once Controller script detects that some company reached 10M value, it should announc with public message stating that this company and player wins. Then after 1 minute after that, it should restart the game.
