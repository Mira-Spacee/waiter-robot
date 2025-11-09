import burgerImg from '@/assets/burger.jpg';
import saladImg from '@/assets/salad.jpg';
import pastaImg from '@/assets/pasta.jpg';
import juiceImg from '@/assets/juice.jpg';
import coffeeImg from '@/assets/coffee.jpg';
import waterImg from '@/assets/water.jpg';
import { MenuItem } from '@/contexts/OrderContext';

export const menuItems: MenuItem[] = [
  {
    id: '1',
    name: 'Classic Burger',
    price: 12.99,
    image: burgerImg,
    category: 'meals'
  },
  {
    id: '2',
    name: 'Caesar Salad',
    price: 9.99,
    image: saladImg,
    category: 'meals'
  },
  {
    id: '3',
    name: 'Pasta Marinara',
    price: 14.99,
    image: pastaImg,
    category: 'meals'
  },
  {
    id: '4',
    name: 'Fresh Orange Juice',
    price: 4.99,
    image: juiceImg,
    category: 'drinks'
  },
  {
    id: '5',
    name: 'Espresso Coffee',
    price: 3.99,
    image: coffeeImg,
    category: 'drinks'
  },
  {
    id: '6',
    name: 'Mineral Water',
    price: 2.99,
    image: waterImg,
    category: 'drinks'
  }
];
