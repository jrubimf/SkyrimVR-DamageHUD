
class Main 
{
	public static function main(swfRoot:MovieClip):Void 
	{
		(DamageWidget)(org.casalib.util.MovieClipUtil.createMovieRegisterClass("DamageWidget.as",swfRoot,"widget",swfRoot.getNextHighestDepth(),{_x:0,_y:0}));
		swfRoot.widget = new DamageWidget(swfRoot);
	}
	
	public function Main() 
	{

	}
	
}