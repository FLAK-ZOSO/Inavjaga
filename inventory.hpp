struct Inventory {
    short clay;
    short bullets;
    short meat;

    void operator+=(const Inventory&);
    void operator-=(const Inventory&);

    inline bool containsAtLeast(const Inventory) const;
}; // The idea is that the inventory can be dropped (as CHEST) and picked up by the player