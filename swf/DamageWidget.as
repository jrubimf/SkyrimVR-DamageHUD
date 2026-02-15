
class DamageWidget
{
	private var widgetHolder:MovieClip;
	
	public var textFont:String = "$EverywhereMediumFont";
	public var textSize:Number = 24;
	public var textAlpha:Number = 100;
	public var blockSize:Number = 24;
	public var sneakSize:Number = 24;
	public var criticalSize:Number = 24;
	public var damageMode:String = "DefaultIns";
	public var healMode:String = "DefaultIns";
	
	public var currentSize:Number = 24;
	
	public function DamageWidget(swfRoot:MovieClip)
	{
		super();
		
		widgetHolder = swfRoot;
		
		Stage.align = "TL";
		Stage.scaleMode = "noScale";
	}
	
	public function GetModeString(a_mode:Number):String
	{
		var result:String;
		
		switch (a_mode)
		{
			case 1:
				result = "FreezeIns";
				break;
			case 2:
				result = "AccelerateIns";
				break;
			case 3:
				result = "BoundIns";
				break;
			case 4:
				result = "DropIns";
				break;
			default:
				result = "DefaultIns";
				break;
		}
		
		return result;
	}
	
	public function SetSettings(a_font:String, a_size:Number, a_alpha:Number, b_size:Number, s_size:Number, c_size:Number, d_mode:Number, h_mode:Number):Void
	{
		textFont = a_font;
		textSize = a_size;
		textAlpha = a_alpha;
		blockSize = b_size;
		sneakSize = s_size;
		criticalSize = c_size;
		damageMode = GetModeString(d_mode);
		healMode = GetModeString(h_mode);
	}
	
	public function SetDamageText(a_damage:String, a_x:Number, a_y:Number, a_scale:Number, a_color:Number, a_flag:Number):Void
	{
		var mc_obj:MovieClip;
		
		if (a_flag < 10)
		{
			mc_obj = widgetHolder.attachMovie(damageMode, "ins", widgetHolder.getNextHighestDepth());
		}
		else
		{
			mc_obj = widgetHolder.attachMovie(healMode, "ins", widgetHolder.getNextHighestDepth());
		}
		
		mc_obj._xscale = mc_obj._yscale = a_scale;
		mc_obj._alpha = textAlpha;
		var format:TextFormat = mc_obj.ins_.getNewTextFormat();
		format.align = "center";
		format.font = textFont;
		
		switch (a_flag)
		{
			case -14:
				format.size = currentSize = criticalSize;
				mc_obj._x = Stage.width * a_x;
				mc_obj._y = Stage.height * a_y;
				break;
			case -13:
				format.size = currentSize = sneakSize;
				mc_obj._x = Stage.width * a_x;
				mc_obj._y = Stage.height * a_y;
				break;
			case -12:
				format.size = currentSize = blockSize;
				mc_obj._x = Stage.width * a_x;
				mc_obj._y = Stage.height * a_y;
				break;
			case -11:
				format.size = sneakSize;
				mc_obj._x = Stage.width * a_x;
				mc_obj._y = Stage.height * a_y + (currentSize + criticalSize) * a_scale / 100;
				break;
			case -10:
				format.size = blockSize;
				mc_obj._x = Stage.width * a_x;
				mc_obj._y = Stage.height * a_y + (currentSize + criticalSize + sneakSize) * a_scale / 100;
				break;
			case -9:
				format.size = blockSize;
				mc_obj._x = Stage.width * a_x;
				mc_obj._y = Stage.height * a_y + (currentSize + sneakSize) * a_scale / 100;
				break;
			case -8:
				format.size = blockSize;
				mc_obj._x = Stage.width * a_x;
				mc_obj._y = Stage.height * a_y + (currentSize + criticalSize) * a_scale / 100;
				break;
			case -7:
				format.size = criticalSize;
				mc_obj._x = Stage.width * a_x;
				mc_obj._y = Stage.height * a_y + currentSize * a_scale / 100;
				break;
			case -6:
				format.size = sneakSize;
				mc_obj._x = Stage.width * a_x;
				mc_obj._y = Stage.height * a_y + currentSize * a_scale / 100;
				break;
			case -5:
				format.size = blockSize;
				mc_obj._x = Stage.width * a_x;
				mc_obj._y = Stage.height * a_y + currentSize * a_scale / 100;
				break;
			case -4:
				format.size = criticalSize;
				mc_obj._x = Stage.width * a_x;
				mc_obj._y = Stage.height * a_y + textSize * a_scale / 100;
				break;
			case -3:
				format.size = sneakSize;
				mc_obj._x = Stage.width * a_x;
				mc_obj._y = Stage.height * a_y + textSize * a_scale / 100;
				break;
			case -2:
				format.size = blockSize;
				mc_obj._x = Stage.width * a_x;
				mc_obj._y = Stage.height * a_y + textSize * a_scale / 100;
				break;
			case -1:
				format.size = textSize;
				mc_obj._x = Stage.width * a_x;
				mc_obj._y = Stage.height * a_y;
				break;
			case 0:
				format.size = currentSize = textSize;
				mc_obj._x = Stage.width * a_x;
				mc_obj._y = Stage.height * a_y;
				break;
			case 1:
				format.size = textSize;
				mc_obj._x = Stage.width * a_x - textSize * a_scale / 50;
				mc_obj._y = Stage.height * a_y;
				break;
			case 2:
				format.size = textSize;
				mc_obj._x = Stage.width * a_x + textSize * a_scale / 50;
				mc_obj._y = Stage.height * a_y;
				break;
			case 10:
				format.size = textSize;
				mc_obj._x = Stage.width * a_x;
				mc_obj._y = Stage.height * a_y - textSize * a_scale / 100;
				break;
			case 11:
				format.size = textSize;
				mc_obj._x = Stage.width * a_x - textSize * a_scale / 50;
				mc_obj._y = Stage.height * a_y - textSize * a_scale / 100;
				break;
			case 12:
				format.size = textSize;
				mc_obj._x = Stage.width * a_x + textSize * a_scale / 50;
				mc_obj._y = Stage.height * a_y - textSize * a_scale / 100;
				break;
			default:
				format.size = textSize;
				mc_obj._x = Stage.width * a_x;
				mc_obj._y = Stage.height * a_y;
				break;
		}
		
		mc_obj.ins_.setNewTextFormat(format);
		mc_obj.ins_.textColor = a_color;
		mc_obj.ins_.autoSize = "center";
		mc_obj.ins_.text = a_damage;
		
		mc_obj.onEnterFrame = function ()
		{
			if (this._currentframe == this._totalframes)
			{
				this.removeMovieClip();
			}
		}
	}
}
