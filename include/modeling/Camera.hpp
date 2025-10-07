#include <Eigen/Core>
#include <Eigen/Dense>

using namespace Eigen;

class Camera {
    public:
        /* move the camera to a target point */
        void setPos(Vector3f pos) {
            this->pos=pos;
            updateView();
        }
        /* add an offset vector to camera position */
        void translate(Vector3f offs) {
            this->pos+=offs;
            updateView();
        }
        void translate(float x, float y, float z) {
            this->pos(0)+=x; this->pos(1)+=y; this->pos(2)+=z;
            updateView();
        }

        void LookAt(Vector3f front);
        void LookAt(float yaw, float pitch);

        void setFOV(float fov) { this->fov=fov; }

        /* rotate the camera facing direction about an axis */
        void rotate(float radians, Vector3f axis);
        /* rotate the camera facing direction about the vertical axis */
        void rotateHori(float radians) { rotate(radians, getUp()); }
        /* rotate the camera facing direction about a horizontal axis */
        void rotateVert(float radians) { rotate(radians, getRight()); }

        Vector3f getPos() { return pos; }
        Vector3f getRight() { return right; }
        Vector3f getUp() { return up; }
        Vector3f getDirection() { return front; }
		Matrix4f getView() { return view; }

        float getFOV() { return fov; }

        Camera(Vector3f pos, Vector3f front);

    private:
        /* coordinates where the camera is */
        Vector3f pos;
        /* direction vector the camera is pointing */
        Vector3f front;
        /* which way is up? can be fixed for now since we probably don't need a flight-style camera */
        Vector3f up;
        /* which way is to the right? calculated from direction */
        Vector3f right;
        /* view matrix */
        Matrix4f view;

        float fov;

        /*
         * helper. any time front or pos changes, we need to call
         * this to sync view, right, and up
         */
        void updateView();

		/*
		 * helper. LookAt matrix implementation
		 */
		static Matrix4f lookat(Vector3f right, Vector3f up, Vector3f direction, Vector3f pos);
};
