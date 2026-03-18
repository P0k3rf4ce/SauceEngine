#include <physics/Collider.hpp>

namespace physics {

    struct BVH : public Collider {
        virtual bool checkCollision(const Collider& collider,
                                    std::vector<ContactInfo>& info) const override;

      private:
        std::shared_ptr<Collider> left, right;
    };

} // namespace physics
