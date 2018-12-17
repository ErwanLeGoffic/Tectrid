using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;

public class BrushParametersUI : MonoBehaviour
{
    [SerializeField] private TextMesh _brushRadiusText;
    [SerializeField] private TextMesh _brushStrengthText;

	public void UpdateBrushRadius(float value)
    {
        _brushRadiusText.text = "Brush Radius : " + value;
    }

    public void UpdateBrushStrength(float value)
    {
        _brushStrengthText.text = "Brush Strength : " + value;
    }
}
