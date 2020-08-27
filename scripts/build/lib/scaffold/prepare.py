import cv2
import numpy as np
import os
import struct

def traverse_folder_and_transform_to_raw(folder, w, h, mean_rgb, var_rgb):
    if not os.path.exists("./data/"):
        os.mkdir("./data/")

    F = open("./data_list.txt", "w")
    count = 0
    for f in os.listdir(folder):
        if f[0] == ".":
            continue

        img = cv2.imread(os.path.join(folder, f))
        img = cv2.resize(img, (w,h))
        img = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)
        img = img.astype(np.float32)

        mean_rgb = np.reshape(mean_rgb, [1,1,3])
        var_rgb = np.reshape(var_rgb, [1,1,3])
        img = img-mean_rgb
        img = img/var_rgb
        
        # save to bin
        data_file = open("./data/%d.bin"%count, "wb")
        for data in img.flatten().tolist():
            data_file.write(struct.pack('f', data))
        data_file.close()

        # add file 
        F.write("./data/%d.bin\n"%count)
        count += 1
    
    F.close()


# # traverse_folder_and_transform_to_raw("/Users/jian/Downloads/coco-imgs/",160,160,[128,128,128],[255,255,255])
# f = open("/Users/jian/Downloads/AAAA/tiny-yolov3-0311-demo/data/10.bin",'rb')
# weight_data_size = 160*160*3
# data_raw = struct.unpack('f'*weight_data_size,f.read(4*weight_data_size))
# # output c x input c x H x W
# img = np.asanyarray(data_raw).reshape((160,160,3))

# img = img + np.array([123.68,116.78,103.94]).reshape((1,1,3))
# img = img.astype(np.uint8)
# #cv2.imshow('a',img)
# # cv2.waitKey()
# cv2.imwrite("./aa.png", img)
# f.close()
