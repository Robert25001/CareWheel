import sys
import cv2
import numpy as np
import mediapipe as mp
import time

def get_gaze_ratio(fl, eye_points, iris_center_idx):
    # Récupération des coordonnées X en pixels (ou normalisées)
    corner_left = fl.landmark[eye_points[0]].x
    corner_right = fl.landmark[eye_points[1]].x
    iris_x = fl.landmark[iris_center_idx].x
    
    # Calcul des distances horizontales
    distance_A = abs(iris_x - corner_left)
    distance_B = abs(corner_right - iris_x)
    
    # Éviter la division par zéro
    if distance_B == 0: 
        return 1.0
        
    return distance_A / distance_B

def is_looking_down(face_landmarks):
    y_iris = face_landmarks.landmark[473].y
    y_sup = face_landmarks.landmark[159].y
    y_inf = face_landmarks.landmark[145].y
    q = (y_iris - y_sup) / (y_inf - y_sup)

    if (q > 0.65):
        return True
    return False

mp_face_mesh = mp.solutions.face_mesh
face_mesh = mp_face_mesh.FaceMesh(refine_landmarks=True, max_num_faces=1)

action = "STOP"
center_time = None
#cap = cv2.VideoCapture("http://192.168.11.158:8080/Video")

while True:
    size_bytes = sys.stdin.buffer.read(4)
    if not size_bytes:
        break

    size = int.from_bytes(size_bytes, 'big')
    data = sys.stdin.buffer.read(size)

    frame = np.frombuffer(data, dtype=np.uint8)
    frame = cv2.imdecode(frame, cv2.IMREAD_COLOR)

    frame = cv2.resize(frame, (320, 240))
    rgb = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
    
    results = face_mesh.process(rgb)

    direction = "CENTER"

    if results.multi_face_landmarks:
        for face_landmarks in results.multi_face_landmarks:
            ratio_oeil_droit = get_gaze_ratio(face_landmarks, [362, 263], 473)

            # --- LES CONDITIONS DE DÉTECTION ---
            SEUIL_DROITE = 0.7
            SEUIL_GAUCHE = 1.4

            if (ratio_oeil_droit < SEUIL_DROITE):
                #direction = "DROITE"
                direction = "GAUCHE"
                #center_time = None
                
            elif (ratio_oeil_droit > SEUIL_GAUCHE):
                #direction = "GAUCHE"
                direction = "DROITE"
                #center_time = None

    if results.multi_face_landmarks:
        face_landmarks = results.multi_face_landmarks[0]

        if is_looking_down(face_landmarks):
            direction = "BAS"
    #if is_looking_down(face_landmarks):
        #direction =  "BAS"        
            
    if (direction == "CENTER"):
        if center_time is None:
            center_time = time.time()
        
        elapsed = time.time() - center_time

        if (elapsed >= 2.0):
            action = "AVANT"
        else:
            action = "WAIT"
    elif direction == "BAS":
        action = "STOP"
        center_time = None


    print(direction)
    sys.stdout.flush()