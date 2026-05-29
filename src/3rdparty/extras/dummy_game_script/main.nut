
class DummyGame extends GSController {

}

function DummyGame::Start() {
	/* Wait for the game to start */
	this.Sleep(1);

	GSLog.Info("[DummyGame::Start] GameScript started");

    while (true)
	{
        this.Sleep(1);
    }

	GSLog.Info("[DummyGame::Start] GameScript finished");

}
