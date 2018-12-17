using UnityEngine;
using CircularBuffer;

public class Profiling : MonoBehaviour
{
	private int _nbDT = 250;
	private int _nbDTAverage = 20;
	CircularBuffer<float> _deltaTimes;
	private bool _turnedOn = false;
	public bool turnedOn { set { _turnedOn = value; } }
	public Material _2DDrawMat;

	void Start()
	{
		_deltaTimes = new CircularBuffer<float>(_nbDT);
		for(int i = 0; i < _deltaTimes.Size; ++i)
			_deltaTimes[i] = Time.deltaTime;
	}

	void Update()
	{
		_deltaTimes.PushFront(Time.deltaTime);
	}

	void OnGUI()
	{
		if(!_turnedOn)
		{
			return;
		}

		float deltaTime = 0.0f;
		for(int i = 0; i < _nbDTAverage; ++i)
			deltaTime += _deltaTimes[i];
		deltaTime /= (float)_nbDTAverage;

		int w = Screen.width, h = Screen.height;

		GUIStyle style = new GUIStyle();

		Rect rect = new Rect(w / 2.3f, 0, w, h * 5 / 100);
		style.alignment = TextAnchor.UpperLeft;
		style.fontSize = h * 5 / 100;
		style.normal.textColor = new Color(1.0f, 1.0f, 0.0f, 1.0f);
		float fps = 1.0f / deltaTime;
		string text = string.Format("{0:0.}", fps);
		GUI.Label(rect, text, style);
	}

	void OnPostRender()
	{
		if(!_turnedOn)
		{
			return;
		}
		if(!_2DDrawMat)
		{
			Debug.LogError("Please Assign a material on the inspector");
			return;
		}
		GL.PushMatrix();
		_2DDrawMat.SetPass(0);
		GL.LoadOrtho();
		GL.Begin(GL.LINES);
		GL.Color(Color.yellow);
		for(int i = 0; i < _deltaTimes.Size - 1; ++i)
		{
			GL.Vertex(new Vector3(((float) i / (float) _deltaTimes.Size), Mathf.Clamp((1.0f / _deltaTimes[i]), 0, 60.0f) / 60.0f, 0));
			GL.Vertex(new Vector3(((float) (i+1) / (float)_deltaTimes.Size), Mathf.Clamp((1.0f / _deltaTimes[i+1]), 0, 60.0f) / 60.0f, 0));
		}
		GL.End();
		GL.PopMatrix();
	}
}
