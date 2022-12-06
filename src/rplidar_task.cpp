#include "rplidar_task.h"
#include <Arduino.h>

using namespace sl;
const double d2r = M_PI / 180.f;
const double angleConv = d2r * 90.f / 16384.f;

void printLidarResults(vector<Point>& points, int num_points)
{
    int i = 0;
    printf("Number of points: %u\n"
        " x\ty\tcluster_id\n"
        "-----------------------------\n"
        , num_points);
    while (i < num_points)
    {
        Serial.printf("%lf\t%lf\t%d\n",
                points[i].x,
                points[i].y,
                points[i].clusterID);
        ++i;
    }
}

void printClusterResults(vector<LidarCluster>& clusters, int numClusters) {
    int i = 0;
    printf("Number of total clusters: %u\n"
        " x\ty\tcluster_id\tvalid\n"
        "-----------------------------\n"
        , numClusters);
    while (i < numClusters)
    {
        Serial.printf("%lf\t%lf\t%d\t%d\n",
                clusters[i].centroid.x,
                clusters[i].centroid.y,
                clusters[i].clusterID,
                clusters[i].valid);
        ++i;
    }
}

void printPylonCandidates(vector<PylonCluster>& pylons, int numPylons) {
    int i = 0;
    printf("Number of total pylon candidates: %u\n", numPylons);
    Serial.printf("-----------------------------\n");
    Serial.printf(" x\ty\tcluster_id\n");
    while (i < numPylons)
    {
        Serial.printf("%lf\t%lf\t%d\n",
                pylons[i].centroid.x,
                pylons[i].centroid.y,
                pylons[i].clusterID);
        ++i;
    }
}

void getLidarBoundary(Zone z, ZonePylon p, float* minDist, float* maxDist, bool eps = false) {
    switch (z) {
        case START_ZONE:
            switch (p) {
                case LOCAL_ZONE_PYLON:
                    *minDist = START_LOCAL_MIN;
                    *maxDist = START_LOCAL_MAX;
                    break;
                case FOREIGN_ZONE_PYLON:
                    *minDist = START_FOREIGN_MIN;
                    *maxDist = START_FOREIGN_MAX;
                    break;
            }
            break;
        case DEPOSIT_ZONE:
            switch (p) {
                case LOCAL_ZONE_PYLON:
                    *minDist = DEPOSIT_LOCAL_MIN;
                    *maxDist = DEPOSIT_LOCAL_MAX;
                    break;
                case FOREIGN_ZONE_PYLON:
                    *minDist = DEPOSIT_FOREIGN_MIN;
                    *maxDist = DEPOSIT_FOREIGN_MAX;
                    break;
            }
            break;
    }
    if (eps) {
        *minDist -= CLUSTER_EPS;
        *maxDist += CLUSTER_EPS;
    }

}

// Find the maximum distance between the four extreme maximum and minimum points of a cluster
float getMaxCrossSectionalDist(Coord2D* pts) {
    float dist = 0.;
    //  yes this is n**2 but n will only ever be 4 so byte me
    for (int i = 0; i < 4; i++) {
        for (int j = (i + 1); j < 4; j++) {
            dist = max(dist, (float)(pow(pts[i].x - pts[j].x, 2) + pow(pts[i].y - pts[j].y, 2)));
        }
    }
    return dist;
}

void filterLidarClusters(vector<Point>& points, int num_points, vector<LidarCluster>& clusters,
        int numClusters, Zone z, ZonePylon p) {
    // get bounding coordinates for pylon lidar pts
    float minDist, maxDist;
    getLidarBoundary(z, p, &minDist, &maxDist);
    // clusters should default to being valid
    for (int i = 0; i < numClusters; i++) {
        clusters[i].valid = true;
        clusters[i].clusterID = i + 1;
    }
    for (int i = 0; i < num_points; i++) {
        // check validity of cluster
        if (!clusters[points[i].clusterID - 1].valid) {
            // skip remaining points of cluster
            continue;
        }
        // check bounds of point
        if (!(points[i].dist > minDist && points[i].dist < maxDist)) {
            // point outside of valid range, mark cluster as invalid
            clusters[points[i].clusterID - 1].valid = false;
            continue;
        }
        // check min max of points
        if (clusters[points[i].clusterID - 1].numPts++ == 0) {
            // initialise cluster min & max points
            clusters[points[i].clusterID - 1].x_min = {points[i].x, points[i].y};
            clusters[points[i].clusterID - 1].y_min = {points[i].x, points[i].y};
            clusters[points[i].clusterID - 1].x_max = {points[i].x, points[i].y};
            clusters[points[i].clusterID - 1].y_max = {points[i].x, points[i].y};
        } else {
            // update min and max coords of cluster
            if (points[i].x < clusters[points[i].clusterID - 1].x_min.x) {
                clusters[points[i].clusterID - 1].x_min = {points[i].x, points[i].y};
            }
            if (points[i].y < clusters[points[i].clusterID - 1].y_min.y) {
                clusters[points[i].clusterID - 1].y_min = {points[i].x, points[i].y};
            }
            if (points[i].x > clusters[points[i].clusterID - 1].x_max.x) {
                clusters[points[i].clusterID - 1].x_max = {points[i].x, points[i].y};
            }
            if (points[i].y > clusters[points[i].clusterID - 1].y_max.y) {
                clusters[points[i].clusterID - 1].y_max = {points[i].x, points[i].y};
            }
        }
        // increment centroid of cluster
        clusters[points[i].clusterID - 1].centroid.x += points[i].x;
        clusters[points[i].clusterID - 1].centroid.y += points[i].y;
    }
    for (int i = 0; i < numClusters; i++) {
        if (!clusters[i].valid) {
            continue;
        }
        Coord2D pts[4] = {clusters[i].x_min, clusters[i].y_min, clusters[i].x_max, clusters[i].y_max};
        // check whether the cross sectional distance of the cluster is within boundaries
        if (getMaxCrossSectionalDist(pts) > (CLUSTER_MAX_LENGTH*CLUSTER_MAX_LENGTH)) {
            clusters[i].valid = false;
            continue;
        }
        // calculate centroid of each cluster
        clusters[i].centroid.x = clusters[i].centroid.x / clusters[i].numPts;
        clusters[i].centroid.y = clusters[i].centroid.y / clusters[i].numPts;
    }
}

int getPylonClusters(vector<Point>& points, Zone z, ZonePylon p, vector<PylonCluster>& pylons) {
    int numClusters;
    int numPylons = 0;
    DBSCAN ds(CLUSTER_MIN_SAMPLES, (CLUSTER_EPS * CLUSTER_EPS), points);
    numClusters = ds.run();
    vector<LidarCluster> clusters(numClusters);
    filterLidarClusters(ds.m_points, ds.getTotalPointSize(), clusters, numClusters, z, p);
    PylonCluster pylon;
    for (int i = 0; i < numClusters; i++) {
        if (!clusters[i].valid) {
            continue;
        }
        pylon.clusterID = clusters[i].clusterID;
        pylon.centroid = {clusters[i].centroid.x, clusters[i].centroid.y};
        pylons.push_back(pylon);
        numPylons++;
    }
    // printClusterResults(clusters, numClusters);
    return numPylons;
}

void rankPylons(vector<PylonCluster> localPylons, vector<PylonCluster> foreignPylons,
        int numLocalPylons, int numForeignPylons) {
    PylonCluster bingo, bongo;
    float minDistance, dist;
    bool lazy = true;
    for (int i = 0; i < numLocalPylons; i++) {
        for (int j = 0; j < numForeignPylons; j++) {
            dist = abs(PYLON_DISTANCE - sqrt(pow(localPylons[i].centroid.x - foreignPylons[j].centroid.x, 2)
                    + pow(localPylons[i].centroid.y - foreignPylons[j].centroid.y, 2)));
            if (lazy || dist < minDistance) {
                minDistance = dist;
                lazy = false;
                bingo = localPylons[i];
                bongo = foreignPylons[j];
            }
        }
    }
}

void getLidarPoints(sl_lidar_response_measurement_node_hq_t* nodes, int count, Zone z, int* numLocalPts,
        int* numForeignPts, vector<Point>& localPts, vector<Point>& foreignPts) {
    float minLocalDist, maxLocalDist;
    getLidarBoundary(z, LOCAL_ZONE_PYLON, &minLocalDist, &maxLocalDist, true);
    float minForeignDist, maxForeignDist;
    getLidarBoundary(z, FOREIGN_ZONE_PYLON, &minForeignDist, &maxForeignDist, true);
    float dist, angle;
    Point p;
    for (int i = 0; i < count; i++) {
        dist = nodes[i].dist_mm_q2/4.0f;

        // filter based on distance
        if (dist > minLocalDist && dist < maxLocalDist) {
            // local point
            angle = nodes[i].angle_z_q14 * angleConv;
            p.x = cos(angle) * dist;
            p.y = sin(angle) * dist * -1;
            p.dist = dist;
            p.clusterID = UNCLASSIFIED;
            localPts.push_back(p);
            (*numLocalPts)++;
        } else if (dist > minForeignDist && dist < maxForeignDist) {
            // foreign pt
            angle = nodes[i].angle_z_q14 * angleConv;
            p.x = cos(angle) * dist;
            p.y = sin(angle) * dist * -1;
            p.dist = dist;
            p.clusterID = UNCLASSIFIED;
            foreignPts.push_back(p);
            (*numForeignPts)++;
        }
    }
}

void rplidar_task(void* params);

ILidarDriver* drv = NULL;

void rplidar_task_init() {
    BaseType_t err;
    // task runs on own core 1 to maximise speed
    err = xTaskCreatePinnedToCore(rplidar_task, "RPLIDAR", RPLIDAR_TASK_STACK_SIZE, NULL, 
            RPLIDAR_TASK_PRIORITY, NULL, 1);

    // Serial.printf("test10\n");
    return;
}

void rplidar_task(void* params) {
    sl_result err;
    HardwareSerial SerialLidar(2);
    ILidarDriver* drv = *createLidarDriver();
    lidarLock = xSemaphoreCreateBinary();
    xSemaphoreGive(lidarLock);
    RPLidarScanEventGroup = xEventGroupCreate();
    err = (drv)->connect(&SerialLidar);
    err = (drv)->startScanExpress(false, 3, 0);
    if (err != SL_RESULT_OK) {
        Serial.printf("LIDAR SETUP FAILED, REBOOTING\n");
        esp_restart();
    }

    sl_result ans;
    sl_lidar_response_measurement_node_hq_t nodes[RPLIDAR_BUFFER_SIZE];
    
    for (;;) {
        size_t count = _countof(nodes);
        vector<Point> localPts;
        vector<Point> foreignPts;
        int numLocalPts = 0;
        int numForeignPts = 0;
        // get the scan data
        EventBits_t uxBits;
        uxBits = xEventGroupWaitBits(RPLidarScanEventGroup, EVT_RPLIDAR_SCAN_GROUP, pdTRUE, pdFALSE, RPLIDAR_DEFAULT_TIMEOUT);
        Zone z = START_ZONE;
        if (uxBits & EVT_RPLIDAR_SCAN_COMPLETE) {
            if (_cached_scan_node_hq_count == 0) {
                Serial.printf("Scan timed out - no data\n");
                continue; //consider as timeout
            }

            if (xSemaphoreTake(lidarLock, LIDAR_TIMEOUT)) {
                size_t size_to_copy = std::min(count, _cached_scan_node_hq_count);
                memcpy(nodes, _cached_scan_node_hq_buf, size_to_copy * sizeof      (sl_lidar_response_measurement_node_hq_t));

                count = size_to_copy;
                _cached_scan_node_hq_count = 0;
                xSemaphoreGive(lidarLock);
            }

            // drv->ascendScanData(nodes, count);
            Serial.printf("GOT %d DATA POINTS\n", count);
            getLidarPoints(nodes, count, z, &numLocalPts, &numForeignPts, localPts, foreignPts);
            
            int numLocalPylons, numForeignPylons;
            vector<PylonCluster> localPylons;
            vector<PylonCluster> foreignPylons;
            numLocalPylons = getPylonClusters(localPts, DEPOSIT_ZONE, LOCAL_ZONE_PYLON, localPylons);
            numForeignPylons = getPylonClusters(foreignPts, DEPOSIT_ZONE, FOREIGN_ZONE_PYLON, foreignPylons);
            printPylonCandidates(localPylons, numLocalPylons);
            printPylonCandidates(foreignPylons, numForeignPylons);
        }
        vTaskDelay(1);
    }
}