import tensorflow as tf
import os
import sys

def hex_to_array(hex_data,var_name):
    c_str =''
    
    c_str += '#ifndef ' + var_name.upper() + '_H\n'
    c_str += '#define ' + var_name.upper() + '_H\n'

    c_str += 'const unsigned int ' + var_name + '_len = ' +  str(len(hex_data)) +';\n'
    
    c_str += 'const unsigned char ' + var_name + '[] = {'
    hex_array=[]
    
    for i, val in enumerate(hex_data):
        hex_str = format(val,'#04x')
        
        if (i+1)<len(hex_data):
            hex_str +=','
        if(i+1)%12 ==0:
            hex_str +='\n'
        hex_array.append(hex_str)
    
    c_str += '\n' + format(''.join(hex_array)) + "\n};\n\n"
    c_str += '#endif  //' + var_name.upper()+ '_H'
    
    return c_str

def main():
    if len(sys.argv) < 3:
        print("Usage python convert_no_quat.py <modelfile(.h5)> <input modelname>")
        return
    arg1 = sys.argv[1] #Model
    arg3 = sys.argv[2] #modelname

    print(f"Argument 1: {arg1}")
    print(f"Argument 3: {arg3}")
    model = tf.keras.models.load_model(arg1)
    converter = tf.lite.TFLiteConverter.from_keras_model(model)
    tflite_model = converter.convert()

    with open(f"{arg3}.h",'w') as file :
        file.write(hex_to_array(tflite_model,arg3))

if __name__ == "__main__":
    main()