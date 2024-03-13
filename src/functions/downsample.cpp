#include <types/PointCloud.hh>
#include <types/Accumulators.hh>
#include <pcl/octree/octree.h>


namespace linkml
{
    PointCloud::Ptr PointCloud::downsample(double leaf_size){


        auto cloud = this->makeShared();

        pcl::octree::OctreePointCloudPointVector<PointCloud::PointType> octree(leaf_size);
        octree.setInputCloud(cloud);
        octree.addPointsFromInputCloud();

        std::vector<pcl::octree::OctreePointCloudPointVector<PointCloud::PointType>::LeafNodeIterator> nodes;
        for (auto it = octree.leaf_depth_begin(), it_end = octree.leaf_depth_end(); it != it_end; ++it)
            nodes.push_back(it);

        PointCloud::Ptr filtered_cloud (new PointCloud);
        filtered_cloud->resize(octree.getLeafCount());

        size_t leaf_count = octree.getLeafCount();
        
        #pragma omp parallel for shared(filtered_cloud, nodes, cloud)
        for (size_t i = 0; i < leaf_count; i++){
            pcl::octree::OctreeContainerPointIndices& container = nodes[i].getLeafContainer();

            pcl::Indices indexVector;
            container.getPointIndices(indexVector);

            Accumulators<PointCloud::PointType>::type acc;

            for (auto const& index : indexVector){
                PointCloud::PointType point = cloud->at(index);
                boost::fusion::for_each (acc, pcl::detail::AddPoint<PointCloud::PointType> (point));
            }

            boost::fusion::for_each (acc, pcl::detail::GetPoint< PointCloud::PointType> (filtered_cloud->at(i), indexVector.size()));

        }

        return filtered_cloud;
    }
} // namespace linkml