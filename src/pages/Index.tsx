import { menuItems } from '@/data/menuItems';
import { MenuItemCard } from '@/components/MenuItemCard';
import { BasketSidebar } from '@/components/BasketSidebar';
import { useOrders } from '@/contexts/OrderContext';
import { Button } from '@/components/ui/button';
import { Link } from 'react-router-dom';
import { UtensilsCrossed } from 'lucide-react';
const Index = () => {
  const {
    addToBasket
  } = useOrders();
  const meals = menuItems.filter(item => item.category === 'meals');
  const drinks = menuItems.filter(item => item.category === 'drinks');
  return <div className="min-h-screen bg-background">
      <header className="sticky top-0 z-10 bg-card border-b shadow-sm">
        <div className="container mx-auto px-4 py-4 flex justify-between items-center">
          <div className="flex items-center gap-2">
            <UtensilsCrossed className="h-6 w-6 text-primary" />
            <h1 className="font-bold text-2xl text-justify my-0 mx-0">Menu</h1>
          </div>
          
        </div>
      </header>

      <main className="container mx-auto px-4 py-8">
        <section className="mb-12">
          <h2 className="text-3xl font-bold mb-6 flex items-center gap-2">
            <span className="text-primary">🍽️</span> Meals
          </h2>
          <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-6">
            {meals.map(item => <MenuItemCard key={item.id} item={item} onAdd={addToBasket} />)}
          </div>
        </section>

        <section>
          <h2 className="text-3xl font-bold mb-6 flex items-center gap-2">
            <span className="text-primary">🥤</span> Drinks
          </h2>
          <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-6">
            {drinks.map(item => <MenuItemCard key={item.id} item={item} onAdd={addToBasket} />)}
          </div>
        </section>
      </main>

      <BasketSidebar />
    </div>;
};
export default Index;