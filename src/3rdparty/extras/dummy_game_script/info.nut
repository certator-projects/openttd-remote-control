
class DummyGame extends GSInfo {
	function GetAuthor()		{ return "dummy"; }
	function GetName()			{ return "Dummy Game"; }
	function GetDescription() 	{ return "Goals with many things! :)"; }
	function GetVersion()		{ return 8; }
	function GetDate()			{ return "2014-01-20"; }
	function CreateInstance()		{ return "DummyGame"; }
	function GetShortName()		{ return "DUMM"; }
	function GetAPIVersion()		{ return "1.3"; }
	function GetUrl()			{ return ""; }
	function GetSettings() {

	}
}

RegisterGS(DummyGame());
