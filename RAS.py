import os
import io
from flask import Flask, request, jsonify
from PIL import Image
import random
import google.generativeai as genai

genai.configure(api_key="AIzaSyAFY_BA35k_Br_Jl1oj6V0v4ce9dawWnpo")
gemini_model = genai.GenerativeModel('gemini-1.5-flash')

app = Flask(__name__)

@app.route('/process', methods=['POST'])
def process_image():
    if 'file' not in request.files:
        return jsonify({"error": "No file provided"}), 400

    file = request.files['file']
    image = Image.open(io.BytesIO(file.read()))

    confidence = random.uniform(0, 1)  # Simulating confidence
    if confidence > 0.85:
        return jsonify({"result": "Processed locally", "confidence": confidence})

    # Send to Gemini for processing
    prompt = """
    You are an expert in identifying types of trash.
    Classify this image into food, plastic, glass, metal, paper, organic, medical, nuclear waste, or Not Trash.
    """
    try:
        response = gemini_model.generate_content([{"mime_type": "image/jpeg", "data": file.read()}, prompt])
        return jsonify({"result": response.text, "confidence": confidence})
    except Exception as e:
        return jsonify({"error": str(e)}), 500

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000)
