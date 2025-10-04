#include <glm/glm.hpp>

class Camera {
    public:
        /* move the camera to a target point */
        void setPos(glm::vec3 pos) {
            this->pos=pos;
            updateView();
        }
        /* add an offset vector to camera position */
        void translate(glm::vec3 offs) {
            this->pos+=offs;
            updateView();
        }
        void translate(float x, float y, float z) {
            this->pos.x+=x; this->pos.y+=y; this->pos.z+=z;
            updateView();
        }
    
        void LookAt(glm::vec3 front);
        void LookAt(float yaw, float pitch);

        void setFOV(float fov) { this->fov=fov; }

        /* rotate the camera facing direction about an axis */
        void rotate(float radians, glm::vec3 axis);
        /* rotate the camera facing direction about the vertical axis */
        void rotateHori(float radians) { rotate(radians, getUp()); }
        /* rotate the camera facing direction about a horizontal axis */
        void rotateVert(float radians) { rotate(radians, getRight()); }

        glm::vec3 getPos() { return pos; }
        glm::vec3 getRight() { return right; }
        glm::vec3 getUp() { return up; }
        glm::vec3 getDirection() { return front; }

        float getFOV() { return fov; }

        Camera(glm::vec3 pos, glm::vec3 front);
    private:
        /* coordinates where the camera is */
        glm::vec3 pos;
        /* direction vector the camera is pointing */
        glm::vec3 front;
        /* which way is up? can be fixed for now since we probably don't need a flight-style camera */
        glm::vec3 up;
        /* which way is to the right? calculated from direction */
        glm::vec3 right;
        /* view matrix */
        glm::mat4 view;

        float fov;

        /*
         * helper. any time front or pos changes, we need to call
         * this to sync view, right, and up
         */
        void updateView();
};