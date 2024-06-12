import os
import sys
import datetime
# import scipy


print(os.getpid())
print(datetime.datetime.now())


sys.path.insert(
    0, '/home/mephisto/repos/linkml_cpp/build/')

import _core
from _core import *


print("LinkML-Py loaded")



def chunks(lst, n):
    """Yield successive n-sized chunks from lst."""
    for i in range(0, len(lst), n):
        yield lst[i:i + n]

#dataset_path = "/home/mephisto/server_data/stray_scans/0ba33d855b/" # CPC Office
#dataset_path = "/home/mephisto/server_data/stray_scans/7092626a22/"
#dataset_path = "/home/mephisto/server_data/stray_scans/665518e46a/"
#dataset_path = "/home/mephisto/server_data/stray_scans/8c0e3c381c/" # New scan small room
dataset_path = "/home/mephisto/server_data/stray_scans/3c670b035f" # New scan 

dataset = Dataset(dataset_path)



tmp_folder = "./clouds/"

# Parse dataset
if (False):
    # Remove temp data
    if not os.path.exists(tmp_folder):
        os.makedirs(tmp_folder)
    else:
        for file in os.listdir(tmp_folder):
            os.remove(os.path.join(tmp_folder, file))
    parse_dataset(dataset, tmp_folder, step=4)

if os.path.exists(tmp_folder):
    clouds = PointCloudsOnDisk(tmp_folder)
    #clouds = PointCloudsInMemory(tmp_folder)

# Annotate
if (False):
    for idx, subset in enumerate(chunks(clouds, 1000)):
        subset.annotate("./yolov8x-seg.onnx", dataset)
        print(f"Annotated {idx+1}th subset of {len(clouds)/1000} subsets")
    
    #clouds.annotate("./yolov8x-seg.onnx", dataset)

#clouds.register()

#name = "./0ba33d855b.pcd"
#name = "./CPH_office_downsampled.pcd"
#name = "./Aarhus_office_downsampled.pcd"
#name = "./one_room_downsampled.pcd"
#name = "/home/mephisto/server_data/test_cloud_boxes.pcd"
#name = "/home/mephisto/server_data/test_cloud_boxes_simple.pcd"
#name = "/home/mephisto/server_data/test_cloud_boxes_two.pcd"
#name = "./8c0e3c381c.pcd"
name = "./3c670b035f.pcd"


#clouds.filter()
#cloud = clouds.merge()
#cloud.save(name)
#cloud.display()
cloud = PointCloud(name)
#cloud.downsample(0.05).save(name)
cloud.display()

#cloud = cloud.clustering().save(name)
##cloud.region_growing().save(name)
#cloud.region_growing(
#    #angle_threshold = float(0.96592583),
#    #plane_dist_threshold = float(0.1),
#    minClusterSize = 500,
#    #early_stop = float(0.3),
#    radius = float(0.3),
#    #interval_0 = float(16),
#    #interval_factor = float(1.5),
#    ).save(name)
#cloud.display()


#cloud.solidify()#.display()#.save(path).display()

#cProfile.runctx("cloud.solidify()", globals(), locals(), "Profile.prof")
#s = pstats.Stats("Profile.prof")
#s.strip_dirs().sort_stats("time").print_stats()
#cloud.display()

print("Done")