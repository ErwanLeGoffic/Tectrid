class Profiler
{
    static _histogramProfiling: boolean = true;

	private _nbDT: number = 250;
    private _nbDTAverage: number = 20;
    private _deltaTimes: number[];
    private _started: boolean = false;
    private _engine: BABYLON.Engine;
	private _debugCanvas: HTMLCanvasElement;
	private _debugCanvasContext: CanvasRenderingContext2D;

	public constructor(engine: BABYLON.Engine)
	{
        this._engine = engine;
        if(Profiler._histogramProfiling == false)
            this._nbDT = this._nbDTAverage;
	}

    public Start()
    {
        if(this._started)
            return;
   
        if(this._deltaTimes == null)
        {
            this._deltaTimes = new Array<number>(this._nbDT);
            for(let i = 0; i < this._deltaTimes.length; ++i)
            {
                this._deltaTimes[i] = 0;
            }
        }

        if(this._debugCanvas == null)
        {
            this._debugCanvas = document.createElement("canvas");
            if(Profiler._histogramProfiling)
            {
                this._debugCanvas.width = this._engine.getRenderWidth();
                this._debugCanvas.height = this._engine.getRenderHeight();
                this._debugCanvas.style.width = "100%";
                this._debugCanvas.style.height = "100%";
            }
            else
            {
                this._debugCanvas.width = 80;
                this._debugCanvas.height = 60;
                this._debugCanvas.style.width = "6%";
                this._debugCanvas.style.height = "8%";
            }
            this._debugCanvas.style.position = "absolute";
            this._debugCanvas.style.pointerEvents = "none";
            this._debugCanvas.style.top = "0px";
            this._debugCanvas.style.left = "0px";
            this._debugCanvasContext = this._debugCanvas.getContext("2d");
        }
        document.body.appendChild(this._debugCanvas);
        this._started = true;
    }

    public Stop()
    {
        if(!this._started)
            return;
        document.body.removeChild(this._debugCanvas);
        this._started = false;
    }

    public Update()
    {
        if(!this._started)
            return;

		// Update delta time storage
		this._deltaTimes.pop();
        this._deltaTimes.unshift(this._engine.getDeltaTime());

		// Update average delta time
        let deltaTime: number = 0.0;
        for(let i = 0; i < this._nbDTAverage; ++i)
		{
			deltaTime += this._deltaTimes[i];
		}
        deltaTime /= this._nbDTAverage;

		// Clear 2D canvas
		let w = this._debugCanvas.width;
		let h = this._debugCanvas.height;
		this._debugCanvasContext.clearRect(0, 0, w, h);

		// Print average framerate
		this._debugCanvasContext.textAlign = "start";
		this._debugCanvasContext.fillStyle = '#ffff00';
		this._debugCanvasContext.font = '50pt Calibri';
        let fps: number = 1000.0 / deltaTime;
		this._debugCanvasContext.fillText(fps.toFixed().toString(), w / 2.1, 55);

		// Draw framerate history
        if(Profiler._histogramProfiling)
        {
            this._debugCanvasContext.strokeStyle = '#ffff00';
            this._debugCanvasContext.beginPath();
            for(let i = 0; i < this._deltaTimes.length; ++i)
            {
                if(i == 0)
                    this._debugCanvasContext.moveTo(w * (i / this._deltaTimes.length), h - (h * (this.Clamp((1000.0 / this._deltaTimes[i]), 0, 60.0) / 60.0)));
                else
                    this._debugCanvasContext.lineTo(w * (i / this._deltaTimes.length), h - (h * (this.Clamp((1000.0 / this._deltaTimes[i]), 0, 60.0) / 60.0)));
            }
            this._debugCanvasContext.stroke();
        }
    }

	private Clamp(a: number, b: number, c: number): number
	{
		return Math.max(b, Math.min(c, a));
	}
}
